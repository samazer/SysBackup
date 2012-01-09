<?php // -*- mode: php; mode: mmm; coding: utf-8-unix; -*-
/**
 *
 * Remote File I/O Operations
 *
 * <p>Utility class to execute filesystem & copy functions on a remote
 *    server.  Extends the remote_filesystem_c class in the
 *    sys-remote-filesystem.inc module to add file SCP to and from the
 *    remote system.
 *
 * 
 * <p><b>Copyright</b>
 *
 * <p>Copyright (c) 2001-2011 by Sam Azer, All Rights Reserved.
 *
 * <p>This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * <p>This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * <p>You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>
 *
 * @author Sam Azer <sam@azertech.net>
 * @version 0.01 (alpha)
 * @package aforms
 * @subpackage lib-sys
 * @license http://www.gnu.org/licenses/agpl.html AGPL
 * @copyright 2011 by Sam Azer, All rights reserved
 * @link http://www.azertech.net/
 * @link http://www.samazer.net/about
 *
 * 
 */



require_once "general-sysutils.inc";
require_once "general-sysexec.inc";
require_once "lib/sys-remote-filesys.inc";



// Default SCP options used when none otherwise specified:
// Use Batch Mode, Preserve file mode bits & Disable the progress meter
define( "REMFILEIO_DEFAULTSCPOPTIONS", "-B -p -q" );



/**
 *
 * @package aforms
 * @subpackage lib-sys
 *
 */
class remote_fileio_c extends remote_filesystem_c
{
   var $scp_binary;
   var $scp_filecopy_keypath;
   var $scp_filecopy_options;
   var $scp_filecopy_userid;

   var $scp_cmd;

   var $teelog; // reference to the current app/teelog object

   var $named_parameters;
   var $np_default_method;
   var $np_default_index;


   function __construct( &$teelog, $options )
   {
      $this->named_parameters = new named_parameters_c( "systemlibs", "sys-remote-fileio" );

      $np_method = $this->np_default_method = "remote-exec";
      $np_index  = $this->np_default_index  = "fileio";

      $this->named_parameters->register( $np_method, $np_index, array( 
         "ssh_binary"            => array( NPDEF, "path/name of OpenSSH ssh executable" ),
         "ssh_localexec_keypath" => array( NPREQ, "path/name of private key for LocalExec operations on the remote system" ),
         "ssh_localexec_options" => array( NPOPT, "Additional ssh command options for LocalExec calls to the remote system" ),
         "ssh_localexec_userid"  => array( NPREQ, "UserID for ssh calls to LocalExec on the remote system" ),

         "scp_binary"            => array( NPDEF, "path/name of OpenSSH scp executable" ),
         "scp_filecopy_keypath"  => array( NPREQ, "path/name of private key for SCP operations on the remote system" ),
         "scp_filecopy_options"  => array( NPOPT, "Additional scp command options for file copy operations to/from the remote system" ),
         "scp_filecopy_userid"   => array( NPREQ, "UserID for scp operations to/from the remote system" ),

         "remote_server"         => array( NPREQ, "Remote server name or IP address" ),
                                                 ));


      $ssh_binary        = isset( $options["ssh_binary"] )            ? $options["ssh_binary"]            : trim(`which ssh`);
      $ssh_keypath       = isset( $options["ssh_localexec_keypath"] ) ? $options["ssh_localexec_keypath"] : "";
      $ssh_options       = isset( $options["ssh_localexec_options"] ) ? $options["ssh_localexec_options"] : "";
      $ssh_user          = isset( $options["ssh_localexec_userid"] )  ? $options["ssh_localexec_userid"]  : "";
      $server            = isset( $options["remote_server"] )         ? $options["remote_server"] : "";

      $this->scp_binary  = isset( $options["scp_binary"] )            ? $options["scp_binary"]            : trim(`which scp`);
      $this->scp_filecopy_keypath = isset( $options["scp_filecopy_keypath"] )  ? $options["scp_filecopy_keypath"]  : "";
      $this->scp_filecopy_options = isset( $options["scp_filecopy_options"] )  ? $options["scp_filecopy_options"]  : "";
      $this->scp_filecopy_userid    = isset( $options["scp_filecopy_userid"] )   ? $options["scp_filecopy_userid"]   : "";

      if ( !isset($options["ssh_binary"]) || !$options["ssh_binary"] )
         $options["ssh_binary"] = $ssh_binary;

      if ( !isset($options["scp_binary"]) || !$options["scp_binary"] )
         $options["scp_binary"] = $this->scp_binary;


      if ( !is_readable( $ssh_keypath ) )
      {
         $option_errors = true;
         $teelog->warning( "Can't read file {$ssh_keypath}" );
      }

      if ( !is_readable( $this->scp_filecopy_keypath ) )
      {
         $option_errors = true;
         $teelog->warning( "Can't read file {$this->scp_filecopy_keypath}" );
      }

      $this->named_parameters->check( $this->np_default_method, 
                                      $this->np_default_index, 
                                      $options );

      parent::__construct( $teelog, $ssh_binary, $ssh_keypath, $ssh_options, $ssh_user, $server );

      if ( !$this->scp_filecopy_options )
         $this->scp_filecopy_options = REMFILEIO_DEFAULTSCPOPTIONS;

      $this->scp_cmd = "exec {$this->scp_binary} -i {$this->scp_filecopy_keypath} {$this->scp_filecopy_options}";
   }


   function remote_ref( $idx = 0 )
   {
      if ( ! $idx )
         return parent::remote_ref($idx);

      return "{$this->scp_filecopy_userid}@{$this->remote_server}";
   }


   function dump_config()
   {
      $this->named_parameters->display_properties( $this, $this->np_default_method,
                                                          $this->np_default_index );
   }


   private function _scp( $cmd )
   {
      $r   = array();

      if ( sysexec( $r, $cmd ) === true )
         $this->teelog->fatal( "unable to exec command: '{$cmd}'" );

      if ( $this->debug_sysexec )
         $this->teelog->tee( "SysExec'd: \"{$r['command']}\"" );

      if ( $r["stderr"] )
         $this->last_error( trim($r["stderr"]) );

      // handle funny exit codes from the proc_open() library
      return ( ( ($r["proc_close"] == -1) && ($r["exitcode"] ==  0) )
            || ( ($r["proc_close"] ==  0) && ($r["exitcode"] == -1) )
            || ( ($r["stderr"]     == "") && ($r["exitcode"] == -1) )
             ) ? false : true;
   }


   function scp_to_remote( $source, $dest )
   {
      return $this->_scp( "{$this->scp_cmd} '{$source}' '{$this->scp_filecopy_userid}@{$this->remote_server}:{$dest}'" );
   }


   function scp_from_remote( $source, $dest )
   {
      return $this->_scp( "{$this->scp_cmd} '{$this->scp_filecopy_userid}@{$this->remote_server}:{$source}' '{$dest}'" );
   }


   function file_put_contents( $fn, $content )
   {
      if ( ($t = get_temp_filename( "remote-" )) === false )
         $this->teelog->fatal( "unable to get a temp file name" );

      if ( file_put_contents( $t, $content ) === false )
         $this->teelog->fatal( "unable to write content to temp file ({$t})" );

      $r = $this->scp_to_remote( $t, $fn );

      if ( unlink( $t ) === false )
         $this->teelog->fatal( "unable to unlink temp file ({$t})" );

      // scp return false on success, file_put_contents() returns #bytes written
      if ( $r == false )
         return strlen( $content );

      // file_put_contents() returns false on failure
      return false;
   }
}



