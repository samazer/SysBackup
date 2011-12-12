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
 * @package server-maint
 * @subpackage sysbackup
 * @license http://www.gnu.org/licenses/agpl.html AGPL
 * @copyright 2011 by Sam Azer, All rights reserved
 * @link http://www.azertech.net/
 * @link http://www.samazer.net/about
 *
 */



class backupvfs_sshremote_c extends backupvfs_baseio_c
{
   static $required_options = array(
                                    );


   var $remote;         // an object of type remote_fileio_c
   var $remconf_file;   // name of file that contains remote configuration info

   var $named_parameters;
   var $np_default_method;
   var $np_default_index;


   function __construct( &$application_object, $options )
   {
      parent::__construct( $application_object );

      $this->named_parameters = new named_parameters_c( "systemlibs", "sys-remote-fileio" );

      $np_method = $this->np_default_method = "remote-exec";
      $np_index  = $this->np_default_index  = "remconf";

      $this->named_parameters->register( $np_method, $np_index, array( 
          "enabled"      => array( 1, NPREQ, "plugin_enabled", "true if the plugin is enabled" ),
          "usehardlinks" => array( 1, NPREQ, "usehardlinks",   "true if remote server supports hardlinks" ),
          "remconf_file" => array( 1, "path/name of the configuration file on remote system" ),
                                                 ));

      $this->plugin_enabled     = false;

      $this->plugin_type        = "sshremote";
      $this->plugin_unique      = true;

      $this->plugin_name        = "vfs-sshremote";
      $this->plugin_title       = "SSH Remote Fileio VFS Plugin";
      $this->plugin_version     = "0.01a";
      $this->plugin_description = "Plugin to copy the backupset to a remote server via SSH/RemoteExec";

      $this->remote = new remote_fileio_c( $this->app, $options );

      $this->hostname     = $this->remote->remote_server;
      $this->keeplocal    = true;

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


   function _vfs_keeplocal()
   {
      // remote storage always keeps files
      return true; 
   }

   function _vfs_usehardlinks()
   {
      // remote storage always keeps files
      return $this->usehardlinks;
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


   function set_debugsysexec( $flg = -1 )
   {
      if ( $flg === -1 )
         return $this->debug_sysexec;

      $flg = parent::set_debugsysexec( $flg );

      $this->remote->set_debugsysexec($flg);

      return $flg;
   }


   /**
    *
    * Get Last Hashlist
    *
    * For the local hashlist we just need to check the backup root
    * path for the name/path of the last backup (which is in a file
    * called FN_PATH_BACKUP_LAST.) If the date for that file is the
    * same as the date for today we need to ignore it and get the
    * previous backup instead (in FN_PATH_BACKUP_PREV.)
    *
    * The procedure is identical for the remote hashlist except that
    * there are no convenient or portable ways of checking files on
    * remote filesystems. One way to proceed is to copy any possible
    * files from the remote root to a temp dir on the local
    * system. From there the procedure is the same as for a local
    * hashlist.
    *
    */
   function _get_last_hashlist( $last_name )
   {
      $last_backup = dir_fn( $this->path_backup, $last_name );

      if ( $this->app->verbose )
         $this->tee( "looking for hashlist in {$last_backup}" );

      if ( $this->remote->rfile_exists( $last_backup ) )
      {
         $last_backup_name = trim($this->remote->rreadfile( $last_backup ));

         if ( $this->app->verbose )
            $this->tee( "last backup file contains {$last_backup_name}" );

         if ( $this->app->verbose )
         {
            $this->tee( "Current Backup Name is {$this->current_backup}" );
            $this->tee( "Last Backup Name is {$last_backup_name}" );
         }

         if ( $last_backup_name != $this->current_backup )
         {
            $fn = dir_fn( $this->path_backup . "/" . $last_backup_name, 
                                                     $this->app->sys_hostname . EXT_HASHLIST );

            if ( $this->app->verbose )
               $this->tee( "looking for hashlist {$fn}" );

            if ( $this->remote->ris_file( $fn ) )
            {
               $this->prev_backup       = $last_backup_name;
               $this->prev_path         = dir_fn( $this->path_backup, $last_backup_name );

               $tmp_file = tempnam( $this->app->sys_local->path_temp, "sysbackup-" );
               if ( $this->remote->scp_from_remote( $fn, $tmp_file ) )
                  $this->fatal( "unable to read '{$fn}'" );

               $this->prev_hashlist     = $this->read_hashlist( $tmp_file );
               $this->prev_hashlist_rev = $this->reverse_hashlist( $this->prev_hashlist );

               if ( unlink( $tmp_file ) === false )
                  $this->fatal( "unable to unlink tmp file '{$prev_backup_fn}" );

               if ( $this->app->verbose )
                  $this->tee( "got hashlist {$fn}" );

               return true;
            } 
              else 
            {
               $this->warning( "hashlist not found in '{$last_backup_name}'" );
               return false;
            }
         }
      }

      return false;
   }


   function save_last_backup_name()
   {
      // get the local file names for the current & prev backup info
      $last_backup_name = $this->app->sys_hostname . EXT_BACKUP_LAST;
      $prev_backup_name = $this->app->sys_hostname . EXT_BACKUP_PREV;

      $last_backup_fn   = dir_fn( $this->path_backup, $last_backup_name );
      $prev_backup_fn   = dir_fn( $this->path_backup, $prev_backup_name );

      // check if a backup was made before the current backup
      if ( $this->remote->rfile_exists( $last_backup_fn ) )
      {
         // Yes, so was it a failed version of this one 
         //      or was it as previous successful run?
         $prev_backupset_name = trim($this->remote->rreadfile( $last_backup_fn ));
         if ( $prev_backupset_name != $this->current_backup )
         {
            // it was a previous successful run, so keep it as the prev. backup
            if ( $this->remote->rfile_exists( $prev_backup_fn ) )
               if ( $this->remote->runlink( $prev_backup_fn ) === false )
                  $this->fatal( "unable to unlink prev. backup file: {$prev_backup_fn}" );

            if ( $this->remote->rrename( $last_backup_fn, $prev_backup_fn ) === false )
               $this->fatal( "unable to rename last backup file: {$last_backup_fn} to {$prev_backup_name}" );
         }
      }

      // keep track of the backup that was just completed
      $tmp_file = tempnam( $this->app->sys_local->path_temp, "sysbackup-" );
      if ( file_put_contents( $tmp_file, $this->current_backup ) === false )
         $this->fatal( "unable to write to temp file {$this->app->sys_hostname}:{$tmp_file}" );

      if ( $this->remote->scp_to_remote( $tmp_file, $last_backup_fn ) !== false )
         $this->fatal( "unable to copy {$this->app->sys_hostname}:{$l} to "
                     . "{$remote_sys}:{$r}" );

      if ( $this->remote->rchmod( $last_backup_fn, $this->app->sys_umaskfiles ) === false )
         $this->fatal( "unable to change mode on file: {$last_backup_fn}" );

      if ( unlink( $tmp_file ) === false )
         $this->fatal( "unable to unlink temp file {$this->app->sys_hostname}:{$tmp_file}" );

      // if we get here we're doing okay
      return false;
   }



   function _vfs_hardlink( $linktarget, $linksource )
   {
      if ( $this->remote->rfile_exists( $linksource ) )
         if ( $this->remote->runlink( $linksource ) === false )
            $this->fatal( "unable to remove expired file {$linksource}" );

      if ( $this->app->verbose )
      {
         $s = str_replace( $this->path_backup, "", $linksource );
         $t = str_replace( $this->path_backup, "", $linktarget );

         $this->tee( "hardlinking {$s} to {$t}" );
      }

      if ( $this->remote->rlink( $linktarget, $linksource ) === false )
      {
         // something seriously wrong here... multiple partitions?
         $this->fatal( "unable to link {$linksource} to {$linktarget}" );
      }

      return false;
   }


   function _folder_check( $folders )
   {
      $problems_found = false;
      foreach ( $folders as $folder )
      {
         if ( !$this->remote->rfile_exists( $folder[0] ) )
         {
            $problems_found = true;
            $this->warning( "The {$folder[1]} folder ({$folder[0]}) was not found" );
         }
           else
         {
            if ( !$this->remote->ris_writable( $folder[0] ) )
            {
               $problems_found = true;
               $this->warning( "The {$folder[1]} folder ({$folder[0]}) is not writable" );
            }

            if ( $folder[3] && (($free_space = (int)$this->remote->rdisk_free_space( $folder[0] )) < $folder[2]) )
            {
               $this->warning( "not enough free disk space in the {$folder[1]} folder ({$folder[0]})" );
               $this->warning( "found " . si_suffix( $free_space ) . ", need " . si_suffix( $folder[2] ));
               $problems_found = true;
            }
         }
      }

      if ( $problems_found )
         $this->fatal( "problems found - please correct and try again" );


      if ( $this->remote->rfile_exists($this->current_path) == true )
      {
         if ( $this->remote->rchmod( $this->current_path, $this->app->sys_umaskfolders ) === false )
            $this->fatal( "unable to change mode on subfolder: {$this->current_path}" );

        /**
         *
         * Most likely the contents of the current backupset, if
         * that's what we're writing now, are the result of a
         * previously failed attempt. As such it's probably best to
         * just overwrite it.
         *
         */
         return false; // no problem
      }

      if ( $this->remote->rmkdir( $this->current_path, $this->app->sys_umaskfolders, true ) === false )
         $this->fatal( "unable to create the local backup folder: {$this->current_path}" );

      if ( $this->remote->rchmod( $this->current_path, $this->app->sys_umaskfolders ) === false )
         $this->fatal( "unable to change mode on subfolder: {$this->current_path}" );
   }




   function vfs_init( $backupset_name )
   {
      $this->set_debugsysexec(  $this->app->set_debugsysexec() );

      $remconf_desc = "{$this->remote->ssh_localexec_userid}@{$this->remote->remote_server}:{$this->remconf_file}";

      if ( $this->app->verbose )
         $this->tee( "loading {$remconf_desc}" );

      if ( ($this->remote->ris_readable( $this->remconf_file ) == false)
        || ($conf = $this->remote->rreadfile( $this->remconf_file )) === false )
         $this->fatal( "can't read remote configuration file \"{$remconf_desc}\"" );

      $conf_lines = explode( "\n", $conf );
      $found_conf = array();
      foreach ( array( "path_temp", "path_backup", "path_rotation" ) as $looking_for )
         foreach ( $conf_lines as $txt_line )
         {
            $assign = explode( "=", $txt_line );
            if ( strtolower(trim($assign[0])) == $looking_for )
            {
               $found_conf[$looking_for] = str_replace( "%HOST%", 
                                                        $this->app->sys_hostname, 
                                                        trim( $assign[1] ) );
               break;
            }
         }

      if ( count( $found_conf ) != 3 )
      {
         $this->warning( "Need all of path_temp=path, path_backup=path & path_rotation=path" );
         $this->fatal( "Missing info in {$remconf_desc}" );
      }

      // found it all - load local vars
      foreach( $found_conf as $p => $v )
         $this->$p = $v;

      return $this->_vfs_folder_check( $backupset_name );
   }


   function vfs_start()
   {
      return $this->get_last_hashlist();
   }


   function vfs_end()
   {
      $hash_fn = $this->app->sys_hostname . EXT_HASHLIST;
      $hard_fn = $this->app->sys_hostname . EXT_HARDLINKED;

      if ( $this->app->verbose )
         $this->tee( "writing hashlist {$hash_fn}" );

      $tmp_file = tempnam( $this->app->sys_local->path_temp, "sysbackup-" );
      $this->write_hashlist( $tmp_file, $this->current_hashlist );
      $target = dir_fn( $this->current_path, $hash_fn );
      if ( $this->remote->scp_to_remote( $tmp_file, $target ) !== false )
         $this->fatal( "unable to copy {$this->app->sys_hostname}:{$tmp_file} to {$remote_sys}:{$target}" );
      if ( unlink( $tmp_file ) === false )
         $this->fatal( "unable to unlink temp file {$this->app->sys_hostname}:{$tmp_file}" );

      if ( $this->app->verbose )
         $this->tee( "writing hardlinked file list {$hard_fn}" );

      $tmp_file = tempnam( $this->app->sys_local->path_temp, "sysbackup-" );
      $this->write_hashlist( $tmp_file, $this->current_hardlinked );
      $target = dir_fn( $this->current_path, $hard_fn );
      if ( $this->remote->scp_to_remote( $tmp_file, $target ) !== false )
         $this->fatal( "unable to copy {$this->app->sys_hostname}:{$tmp_file} to {$remote_sys}:{$target}" );
      if ( unlink( $tmp_file ) === false )
         $this->fatal( "unable to unlink temp file {$this->app->sys_hostname}:{$tmp_file}" );

      if ( $this->app->verbose )
         $this->tee( "writing last/prev backup names" );

      $this->save_last_backup_name();

      // always returns no error result
      return false; 
   }


   function vfs_subfolder( $folder_name )
   {
      $fn = dir_fn($this->current_path . "/" . $folder_name);
      if ( $this->remote->rfile_exists( $fn ) )
         return false;

      if ( $this->remote->rmkdir( $fn, $this->app->sys_umaskfolders, true ) === false )
         $this->fatal( "unable to create subfolder: {$fn}" );

      if ( $this->remote->rchmod( $fn, $this->app->sys_umaskfolders ) === false )
         $this->fatal( "unable to change mode on subfolder: {$fn}" );

      return false;
   }



   function remote_canhardlink( $sha1 )
   {
      if ( $this->app->verbose )
      {
         $f = ($this->_vfs_keeplocal() && $this->_vfs_usehardlinks())
            ? "true" : "false";

         $this->tee( "Checking if file can be hardlinked; basic flags say {$f}" );

         $f = ( isset( $this->prev_hashlist_rev[$sha1]    )
             || isset( $this->current_hashlist_rev[$sha1] ) )
            ? "true" : "false";

         $this->app->tee( "found a file on with the same hash? {$f}" );

         $f = ($this->_vfs_keeplocal()
            && $this->_vfs_usehardlinks()
            && ( isset( $this->prev_hashlist_rev[$sha1]    )
              || isset( $this->current_hashlist_rev[$sha1] ) 
                 ))
            ? "true" : "false";

         $this->app->tee( "overall result: {$f}" );
      }

      // can we use a hardlink for this file?
      return $this->_vfs_keeplocal()
          && $this->_vfs_usehardlinks()
          && ( isset( $this->prev_hashlist_rev[$sha1]    )
            || isset( $this->current_hashlist_rev[$sha1] ) 
             )
             ;
   }



   function remote_copyfile( $target, $sha1, $size )
   {
      $rel_ref    = dir_fn( $this->app->current_subfolder       . "/" . $target );
      $local_ref  = dir_fn( $this->app->sys_local->current_path . "/" . $rel_ref );
      $remote_ref = dir_fn( $this->current_path                 . "/" . $rel_ref );

      $remote_sys = $this->remote->remote_ref(1);

      // use a hardlink if possible
      if ( $this->remote_canhardlink( $sha1 ) )
         return $this->_vfs_addfile( $target, $sha1, $size );

      $l = str_replace( $this->app->sys_local->path_backup, "", $local_ref );
      $r = str_replace( $this->path_backup, "", $remote_ref );

      if ( $this->app->verbose )
         $this->tee( "scp {$l} to {$r}" );

      if ( $this->remote->scp_to_remote( $local_ref, $remote_ref ) !== false )
         $this->fatal( "unable to copy {$this->app->sys_hostname}:{$l} to "
                     . "{$remote_sys}:{$r}" );

      if ( $this->remote->rchmod( $remote_ref, $this->app->sys_umaskfiles ) === false )
         $this->fatal( "unable to change mode on file: {$remote_sys}:{$r}" );

      return $this->_vfs_addfile( $target, $sha1, $size );
   }
}



