<?php // -*- mode: php; mode: mmm; coding: utf-8-unix; -*-
/**
 *
 * Remote Filesystem Operations
 *
 * <p>Utility class to execute filesystem functions on a remote server.
 *
 * <p>At the time of this writing, this class executes the localexec
 * filesystem commands, via ssh, on a remote computer. Results are
 * returned in a manner similar to that of the corresponding
 * filesystem functions provided by PHP.
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
 * @subpackage lib-general
 * @license http://www.gnu.org/licenses/agpl.html AGPL
 * @copyright 2011 by Sam Azer, All rights reserved
 * @link http://www.azertech.net/
 * @link http://www.samazer.net/about
 *
 */


define( "LOCALEXEC_TXTRESULT", "Result: " );


require_once "general-sysutils.inc";
require_once "general-sysexec.inc";
require_once "lib/sys-remote-filesys.inc";



class remote_filesystem_c
{
   var $ssh_binary;
   var $ssh_localexec_keypath;
   var $ssh_localexec_options;
   var $ssh_localexec_userid;
   var $remote_server;

   var $teelog;

   var $ssh_cmd;

   var $last_error_text;

   var $debug_sysexec;



   function __construct( &$teelog, $binary, $keypath, $options, $user, $server )
   {
      $this->debug_sysexec = false;

      $this->teelog                = &$teelog;

      $this->ssh_binary            = $binary;
      $this->ssh_localexec_keypath = $keypath;
      $this->ssh_localexec_options = $options;
      $this->ssh_localexec_userid  = $user;
      $this->remote_server         = $server;

      $this->ssh_cmd               = "{$binary} -i {$keypath} {$options} {$user}@{$server} ";

   }



   function remote_ref( $idx = 0 )
   {
      return "{$this->ssh_localexec_userid}@{$this->remote_server}";
   }


   function set_debugsysexec( $flg = -1 )
   {
      if ( $flg === -1 )
         return $this->debug_sysexec;

      return $this->debug_sysexec = $flg;
   }


   function last_error( $t = false )
   {
      if ( $t === false )
         return $this->last_error_text;


      return $this->last_error_text = $t;
   }



   private function _result_stderr( $stdout, $stderr )
   {
      if ( $stderr > "" )
      {
         $this->last_error( "unknown error" );
         $this->teelog->tee( trim($stderr) );
         return false;
      }
   }


   private function _result_val( $stdout, $stderr )
   {
      $this->_result_stderr( $stdout, $stderr );

      if ( substr( $stdout, 0, strlen( LOCALEXEC_TXTRESULT ) ) == LOCALEXEC_TXTRESULT )
         return urldecode(trim(substr($stdout, strlen(LOCALEXEC_TXTRESULT))));

      return trim( $stdout );
   }


   private function _result_valarray( $stdout, $stderr, $keyval )
   {
      $this->_result_stderr( $stdout, $stderr );

      if ( substr( $stdout, 0, strlen( TEELOG_TXTFATAL ) ) == TEELOG_TXTFATAL )
      {
         $this->last_error( trim(substr( $stdout, strlen( TEELOG_TXTFATAL ) ) ) );
         return false;
      }

      if ( substr( $stdout, 0, strlen( LOCALEXEC_TXTRESULT ) ) == LOCALEXEC_TXTRESULT )
         $stdout = substr( $stdout, strpos( $stdout, "\n" ) );

      $vals = explode( "\n", trim($stdout) );

      $r = array();
      foreach( $vals as $line )
         if ( $keyval )
         {
            $info = explode( ":", $line );

            $i_s = trim($info[0]);
            $i_i = (int)(trim($info[0]));
            $v   = trim($info[1]);

            if ( $i_s == "{$i_i}" )
               $r[$i_i] = $v;
            else $r[$i_s] = $v;
         }
           else $r[] = $line;

      return $r;
   }


   private function _result_text( $stdout, $stderr )
   {
      $this->_result_stderr( $stdout, $stderr );

      if ( substr( $stdout, 0, strlen( LOCALEXEC_TXTRESULT ) ) == LOCALEXEC_TXTRESULT )
      {
         if ( ($p = strpos( $stdout, "\n" )) === false )
            return "";

         return substr( $stdout, $p+1 );
      }

      return trim( $stdout );
   }


   private function _exec( $func, $param1, $param2 = "", $param3 = "" )
   {
      $p1 = urlencode(addslashes( $param1 ));
      $p2 = urlencode(addslashes( $param2 ));
      $p3 = urlencode(addslashes( $param3 ));

      $output = array();
      $cmd    = $this->ssh_cmd . "{$func} {$p1} {$p2} {$p3}";
      if ( ($r = sysexec( $output, trim($cmd) )) === true )
         $this->teelog->tee( "unable to exec '{$cmd}'" );

      if ( $this->debug_sysexec )
         $this->teelog->tee( "SysExec'd: \"{$output['command']}\"" );

      return $output;
   }

   private function _exec_val( $func, $p1, $p2 = "", $p3 = "" )
   {
      $output = $this->_exec( $func, $p1, $p2, $p3 );

      return $this->_result_val( $output["stdout"], $output["stderr"] );
   }

   private function _exec_text( $func, $p1, $p2 = "", $p3 = "" )
   {
      $output = $this->_exec( $func, $p1, $p2, $p3 );

      return $this->_result_text( $output["stdout"], $output["stderr"] );
   }

   private function _exec_valarray( $func, $p1, $p2 = "", $p3 = "" )
   {
      $output = $this->_exec( $func, $p1, $p2, $p3 );

      return $this->_result_valarray( $output["stdout"], $output["stderr"], true );
   }

   private function _exec_vallist( $func, $p1, $p2 = "", $p3 = "" )
   {
      $output = $this->_exec( $func, $p1, $p2, $p3 );

      return $this->_result_valarray( $output["stdout"], $output["stderr"], false );
   }

   private function _exec_bool( $func, $p1, $p2 = "", $p3 = "" )
   {
      $output = $this->_exec( $func, $p1, $p2, $p3 );

      if ( substr( $output["stdout"], 0, strlen( TEELOG_TXTFATAL ) ) == TEELOG_TXTFATAL )
      {
         $this->last_error( trim( substr( $output["stdout"], strlen( TEELOG_TXTFATAL ) ) ) );
         return false;
      }

      if ( strtoupper($output["stdout"]) == strtoupper(LOCALEXEC_TXTRESULT . "true\n") )
         return true;

      if ( strtoupper($output["stdout"]) == strtoupper(LOCALEXEC_TXTRESULT . "false\n") )
         return false;

      return false;
   }



   function rdisk_free_space( $path )
   {
      return $this->_exec_val( "disk_free_space", $path );
   }

   function rdisk_total_space( $path )
   {
      return $this->_exec_val( "disk_total_space", $path );
   }

   function rrealpath( $path )
   {
      return $this->_exec_val( "realpath", $path );
   }

   function rreadlink( $path )
   {
      return $this->_exec_val( "readlink", $path );
   }

   function rfilesize( $filespec )
   {
      return $this->_exec_val( "filesize", $filespec );
   }

   function rfiletype( $filespec )
   {
      return $this->_exec_val( "filetype", $filespec );
   }

   function rfileatime( $filespec )
   {
      return $this->_exec_val( "fileatime", $filespec );
   }

   function rfilectime( $filespec )
   {
      return $this->_exec_val( "filectime", $filespec );
   }

   function rfilemtime( $filespec )
   {
      return $this->_exec_val( "filemtime", $filespec );
   }

   function rfileperms( $filespec )
   {
      return $this->_exec_val( "fileperms", $filespec );
   }

   function rlinkinfo( $filespec )
   {
      return $this->_exec_val( "linkinfo", $filespec );
   }

   function rfilegroup( $filespec )
   {
      return $this->_exec_val( "filegroup", $filespec );
   }

   function rfileowner( $filespec )
   {
      return $this->_exec_val( "fileowner", $filespec );
   }

   function rreadfile( $filespec )
   {
      return $this->_exec_text( "readfile", $filespec );
   }



   function rfile_exists( $filespec )
   {
      return $this->_exec_bool( "file_exists", $filespec );
   }

   function ris_dir( $filespec )
   {
      return $this->_exec_bool( "is_dir", $filespec );
   }

   function ris_file( $filespec )
   {
      return $this->_exec_bool( "is_file", $filespec );
   }

   function ris_link( $filespec )
   {
      return $this->_exec1p1bool( "is_link", $filespec );
   }

   function ris_readable( $filespec )
   {
      return $this->_exec_bool( "is_readable", $filespec );
   }

   function ris_writable( $filespec )
   {
      return $this->_exec_bool( "is_writable", $filespec );
   }

   function ris_executable( $filespec )
   {
      return $this->_exec_bool( "is_executable", $filespec );
   }



   function rchgrp( $filename, $group )
   {
      return $this->_exec_bool( "chgrp", $filename, $group );
   }

   function rchmod( $filename, $mode )
   {
      return $this->_exec_bool( "chmod", $filename, $mode );
   }

   function rchown( $filename, $user )
   {
      return $this->_exec_bool( "chown", $filename, $user );
   }

   function rlchgrp( $filename, $group )
   {
      return $this->_exec_bool( "lchgrp", $filename, $group );
   }

   function rlchown( $filename, $user )
   {
      return $this->_exec_bool( "lchown", $filename, $user );
   }


   function rglob( $cwd, $pattern, $flags )
   {
      return $this->_exec_vallist( "glob", $cwd, $pattern, $flags );
   }


   function rstat( $filespec )
   {
      return $this->_exec_valarray( "stat", $filespec );
   }


   function rlstat( $filespec )
   {
      return $this->_exec_valarray( "lstat", $filespec );
   }


   function rcopy( $source, $dest )
   {
      return $this->_exec_bool( "copy", $source, $dest );
   }


   function rrename( $source, $dest )
   {
      return $this->_exec_bool( "rename", $source, $dest );
   }


   function rlink( $target, $link )
   {
      return $this->_exec_bool( "link", $target, $link );
   }


   function rsymlink( $target, $link )
   {
      return $this->_exec_bool( "symlink", $target, $link );
   }


   function rmkdir( $path )
   {
      return $this->_exec_bool( "mkdir", $path );
   }


   function rrmdir( $path )
   {
      return $this->_exec_bool( "rmdir", $path );
   }


   function runlink( $filespec )
   {
      return $this->_exec_bool( "unlink", $filespec );
   }
}



