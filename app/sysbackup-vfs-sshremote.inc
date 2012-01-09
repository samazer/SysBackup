<?php // -*- mode: php; mode: mmm; coding: utf-8-unix; -*-
/** 
 *
 * Virtual Backup Filesystem plugin - remote filesystem via ssh/remexec/localexec
 *
 *
 * <p><b>Copyright</b>
 *
 * <p>Copyright (c) 2011 by Sam Azer, All Rights Reserved.
 *
 * <p>This program is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
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
 * @package sysbackup
 * @subpackage sysbackup
 * @license http://www.gnu.org/licenses/agpl.html AGPL
 * @copyright 2011 by Sam Azer, All rights reserved
 * @link http://www.azertech.net/
 * @link http://www.samazer.net/about
 *
 */



/**
 *
 * SSH I/O support
 *
 * This class extends the Base VFS I/O class and overrides the methods
 * that access the filesystem. The new methods use the remote file I/O
 * class to perform the desired operations on the remote system
 * through SSH.
 *
 */
class backupvfs_sshremote_io_c extends backupvfs_baseio_c
{
   var $remote;         // an object of type remote_fileio_c

   function __construct( &$application_object, $options )
   {
      parent::__construct( $application_object );

      $this->remote       = new remote_fileio_c( $this->app, $options );
      $this->hostname     = $this->remote->remote_server;
      $this->keeplocal    = true;
   }


   function get_keeplocal()
   {
      // remote storage always keeps files
      return true; 
   }


   function job_reset()    { $this->backupset->job_reset();  }
   function step_reset()   { $this->backupset->step_reset(); }
   function all_reset()    { $this->backupset->job_reset();
                             $this->backupset->step_reset(); }

   function get_stats()    { return $this->backupset->get_stats();  }


  /**
   *
   * Fatal File Error
   *
   * This method tries to produce a meaningful error message in the
   * event of an error on the filesystem.
   *
   */
   function _vfs_fatal_file_error( $msg, $source = false, $target = false  )
   {
      if ( $source !== false )
         $this->warning( "source file is {$source}" );

      if ( $target !== false )
         $this->warning( "target file is {$target}" );

      $r = $this->remote->last_error();
      if ( $r )
         $this->warning( "remote filesystem error message: {$r}" );

      $this->fatal( $msg );
   }


   /**
    *
    * Check if a file exists
    *
    * This is one of the basic operations that needs to be done on a
    * VFS in order to get the previously written hashlist. Each VFS
    * which extends this class will have to override this method to
    * provide this functionality on the target filesystem. 
    *
    * For convenience sake the base class implements the functionality
    * required for a simple disk-based filesystem.
    *
    */
   function _vfs_file_exists( $fn )
   {
      return $this->remote->file_exists( $fn );
   }


   /**
    *
    * return the contents of a small file
    *
    */
   function _vfs_file_get_contents( $fn )
   {
      return $this->remote->readfile( $fn );
   }


   /**
    *
    * unlink/delete a small file
    *
    */
   function _vfs_unlink( $fn )
   {
      return $this->remote->unlink( $fn );
   }


   /**
    *
    * hardlink to an existing file
    *
    */
   function _vfs_link( $target, $source )
   {
      return $this->remote->link( $target, $source );
   }


   /**
    *
    * return the contents of a small file
    *
    */
   function _vfs_rename( $from, $to )
   {
      return $this->remote->rename( $from, $to );
   }


   /**
    *
    * Write text to a small file
    *
    */
   function _vfs_file_put_contents( $fn, $content )
   {
      return $this->remote->file_put_contents( $fn, $content );
   }


   /**
    *
    * Change the permissions of a file
    *
    */
   function _vfs_chmod( $fn, $mask )
   {
      return $this->remote->chmod( $fn, $mask );
   }


   /**
    *
    * Change the permissions of a file
    *
    */
   function _vfs_mkdir( $dir )
   {
      return $this->remote->mkdir( $dir );
   }


   /**
    *
    * determine if write permission is available
    *
    */
   function _vfs_is_writable( $dir )
   {
      return $this->remote->is_writable( $dir );
   }


   /**
    *
    * determine the amount of free space available
    *
    */
   function _vfs_disk_free_space( $dir )
   {
      return $this->remote->disk_free_space( $dir );
   }


   /**
    *
    * return the size of a file
    *
    */
   function _vfs_filesize( $fn )
   {
      return $this->remote->filesize( $fn );
   }


   /**
    *
    * determine the total amount of disk space available
    *
    */
   function _vfs_disk_total_space( $dir )
   {
      return $this->remote->disk_total_space( $dir );
   }


   /**
    *
    * calculate an sha1 hash for a file
    *
    */
   function _vfs_sha1_file( $fn )
   {
      return $this->remote->sha1_file( $fn );
   }


   /**
    *
    * Determine, using brute force, if it is possible to hardlink
    * between these two folders
    *
    */
   function _vfs_can_hardlink( $target_dir, $source_dir )
   {
      return $this->remote->can_hardlink( $target_dir, $source_dir );
   }


   /**
    *
    * List items in a folder that have expired
    *
    */
   function _vfs_list_expired_items( $path, $max )
   {
      return $this->remote->list_expired_items( $path, $max );
   }


   /**
    *
    * Remove items in a folder that have expired
    *
    */
   function _vfs_remove_expired_items( $path, $max, $debug = true )
   {
      return $this->remote->remove_expired_items( $path, $max, $debug );
   }
}




/**
 *
 * SSH VFS
 *
 * This class extends the I/O class to provide the remote backupset VFS logic.
 *
 */
class backupvfs_sshremote_c extends backupvfs_sshremote_io_c 
{
   var $remconf_file;   // name of file that contains remote configuration info

   var $named_parameters;
   var $np_default_method;
   var $np_default_index;


   function __construct( &$application_object, $options )
   {
      parent::__construct( $application_object, $options );

      $this->plugin_enabled     = false;

      $this->plugin_type        = "sshremote";
      $this->plugin_unique      = true;

      $this->plugin_name        = "vfs-sshremote";
      $this->plugin_title       = "SSH Remote Fileio VFS Plugin";
      $this->plugin_version     = "0.01a";
      $this->plugin_description = "Plugin to copy the backupset to a remote server via SSH/RemoteExec";

      $this->named_parameters = new named_parameters_c( "systemlibs", "sys-remote-fileio" );

      $np_method = $this->np_default_method = "remote-exec";
      $np_index  = $this->np_default_index  = "remconf";

      $this->named_parameters->register( $np_method, $np_index, array( 
          "enabled"      => array( 1, NPREQ, "plugin_enabled", "true if the plugin is enabled" ),
          "usehardlinks" => array( 1, NPREQ, "usehardlinks",   "true if remote server supports hardlinks" ),
          "remconf_file" => array( 1, "path/name of the configuration file on remote system" ),
                                                 ));

      $this->named_parameters->set_optional(   $this->np_default_method, 
                                               $this->np_default_index, 
                                               $options );

      $this->named_parameters->check(          $this->np_default_method, 
                                               $this->np_default_index, 
                                               $options );

      $this->named_parameters->set_properties( $this,
                                               $this->np_default_method, 
                                               $this->np_default_index, 
                                               $options );
   }



   function dump_config()
   {
      parent::dump_config();

      $this->named_parameters->display_properties( $this, 
                                                   $this->np_default_method, 
                                                   $this->np_default_index ); 

      $this->remote->dump_config( $this );
      $this->app->tee("");
   }



   function plugin_help()  
   {
      $this->tee( "(help for this plugin is not available yet)" );
   }


   /**
    *
    * Plugin Instance
    *
    * return a string of the form:
    *
    * SSH remote VFS (user@server-name.tld)
    *
    */
   function plugin_instance()
   {
      $desc = $this->remote->remote_ref();
      return "Remote VFS on {$desc}";
   }


   function plugin_shortref()
   {
      return $this->remote->remote_ref();
   }



   function set_debugsysexec( $flg = -1 )
   {
      if ( $flg === -1 )
         return $this->debug_sysexec;

      $flg = parent::set_debugsysexec( $flg );

      $this->remote->set_debugsysexec($flg);

      return $flg;
   }



   function load_remote_configuration_file()
   {
      $remconf_desc = "{$this->remote->ssh_localexec_userid}@{$this->remote->remote_server}:{$this->remconf_file}";

      if ( $this->app->verbose() )
         $this->tee( "loading {$remconf_desc}" );

      if ( ($this->remote->is_readable( $this->remconf_file ) == false)
        || ($conf = $this->remote->file_get_contents( $this->remconf_file )) === false )
         $this->fatal( "can't read remote configuration file \"{$remconf_desc}\"" );

      $conf_lines = explode( "\n", $conf );
      $found_conf = array();
      $looking_for_list = array( "path_temp", "path_daily", "path_weekly", "path_monthly" );

      $looking_for_text = "";
      foreach ( $looking_for_list as $t )
         $looking_for_text = $looking_for_text
                           . ($looking_for_text > "" ? ", " : "")
                           . "{$t}=path"
                           ;

      foreach ( $looking_for_list as $looking_for )
         foreach ( $conf_lines as $txt_line )
         {
            $assign = explode( "=", $txt_line );
            if ( strtolower(trim($assign[0])) == $looking_for )
            {
               $found_conf[$looking_for] = str_replace( "%HOST%", 
                                                        $this->app->get_hostname(),
                                                        trim( $assign[1] ) );
               break;
            }
         }

      if ( count( $found_conf ) != count( $looking_for_list ) )
      {
         $this->warning( "Need all of ({$looking_for_text})" );
         $this->fatal( "Missing info in {$remconf_desc}" );
      }

      // found it all - load local vars
      foreach( $found_conf as $p => $v )
         $this->$p = $v;
   }



   function vfs_init( $backupset_name )
   {
      $this->set_debugsysexec(  $this->app->set_debugsysexec() );

      $this->backupset_name = $backupset_name;

      $this->load_remote_configuration_file();

      $problems_found = $this->vfs_warn_disk_space( true );
      if ( !$this->app->get_keeplocal() && $problems_found )
         $this->fatal( "problems found - please correct and try again" );

      return $this->vfs_createbackupset_folder( $this->path_daily );
   }



   function vfs_prune_expired_backups()
   {
      $description    = $this->get_hostname() . " daily";
      $path_to_prune  = $this->get_path_daily();
      $max_items      = $this->get_rotations_daily();

      return $this->_vfs_prune_expired_backups( $description, $path_to_prune, $max_items );
   }



   function remote_copyfile( $rel_ref, $sha1, $size )
   {
      $local_ref  = dir_fn( $this->app->get_path_backupset(), $rel_ref );
      $remote_ref = dir_fn( $this->get_path_backupset(),      $rel_ref );

      // find out if the file should be hardlinked somewhere?
      if ( ($where = $this->backupset->should_hardlink( $sha1 )) == false )
      {
         // nope - simple copy only
         $l = $this->vfs_displayref( $local_ref );
         $r = $this->vfs_displayref( $remote_ref );

         if ( $this->app->verbose() )
            $this->tee( "scp {$l} to {$r}" );

         if ( $this->remote->scp_to_remote( $local_ref, $remote_ref ) !== false )
            $this->fatal( "unable to copy {$l} to {$r}" );

         if ( $this->remote->chmod( $remote_ref, $this->get_umaskfiles() ) === false )
            $this->fatal( "unable to change mode on file {$r}" );
      }

      $this->backupset->_register( $where, $rel_ref, $sha1, $size );
      return false;
   }
}



