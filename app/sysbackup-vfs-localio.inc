<?php // -*- mode: php; mode: mmm; coding: utf-8-unix; -*-
/** 
 *
 * Virtual Backup Filesystem plugin - local filesystem
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
 * <p>You should have received a copy of the GNU Affero General Public
 * License along with this program.  If not, see
 * <http://www.gnu.org/licenses/>
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



class backupvfs_localio_c extends backupvfs_baseio_c
{
   function __construct( &$application_object )
   {
      parent::__construct( $application_object );

      $this->plugin_enabled     = false;

      $this->plugin_type        = "vfslocalio";
      $this->plugin_unique      = true;

      $this->plugin_name        = "vfs-localio";
      $this->plugin_title       = "Local Fileio VFS Plugin";
      $this->plugin_version     = "0.01a";
      $this->plugin_description = "Plugin to write the backupset to the local filesystem";
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
      return "Local VFS on {$this->hostname}";
   }


   function plugin_shortref()
   {
      return $this->hostname;
   }


   function _vfs_keeplocal()
   {
      return $this->app->sys_keeplocal;
   }


   function _vfs_usehardlinks()
   {
      // remote storage always keeps files
      return $this->app->sys_usehardlinks;
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

      if ( file_exists( $last_backup ) )
      {
         $last_backup_name = trim(file_get_contents( $last_backup ));

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

            if ( is_file( $fn ) )
            {
               $this->prev_backup       = $last_backup_name;
               $this->prev_path         = dir_fn( $this->path_backup, $last_backup_name );
               $this->prev_hashlist     = $this->read_hashlist( $fn );
               $this->prev_hashlist_rev = $this->reverse_hashlist( $this->prev_hashlist );

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


   function _folder_check( $folders )
   {
      $problems_found = false;
      foreach ( $folders as $folder )
      {
         if ( !file_exists( $folder[0] ) )
         {
            $problems_found = true;
            $this->warning( "The {$folder[1]} folder ({$folder[0]}) was not found" );
         }
           else
         {
            if ( !is_writable( $folder[0] ) )
            {
               $problems_found = true;
               $this->warning( "The {$folder[1]} folder ({$folder[0]}) is not writable" );
            }

            if ( $folder[3] && (($free_space = disk_free_space( $folder[0] )) < $folder[2]) )
            {
               $this->warning( "not enough free disk space in the {$folder[1]} folder ({$folder[0]})" );
               $this->warning( "found " . si_suffix( $free_space ) . ", need " . si_suffix( $folder[2] ));
               $problems_found = true;
            }
         }
      }

      if ( $problems_found )
         $this->fatal( "problems found - please correct and try again" );


      if ( file_exists($this->current_path) == true )
      {
         if ( chmod( $this->current_path, $this->app->sys_umaskfolders ) === false )
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

      if ( mkdir( $this->current_path, $this->app->sys_umaskfolders, true ) === false )
         $this->fatal( "unable to create the local backup folder: {$this->current_path}" );

      if ( chmod( $this->current_path, $this->app->sys_umaskfolders ) === false )
         $this->fatal( "unable to change mode on subfolder: {$this->current_path}" );
   }


   function vfs_init( $backupset_name )
   {
      return $this->_vfs_folder_check( $backupset_name );
   }


   function vfs_subfolder( $folder_name )
   {
      $fn = dir_fn($this->current_path . "/" . $folder_name);
      if ( file_exists( $fn ) )
         return false;

      if ( mkdir( $fn, $this->app->sys_umaskfolders, true ) === false )
         $this->fatal( "unable to create subfolder: {$fn}" );

      if ( chmod( $fn, $this->app->sys_umaskfolders ) === false )
         $this->fatal( "unable to change mode on subfolder: {$fn}" );

      return false;
   }


   function _fatal_file_error( $msg, $source = false, $target = false  )
   {
      $r = $this->app->tar->last_exec_result();

      if ( $source !== false )
         $this->warning( "source file is {$source}" );

      if ( $target !== false )
         $this->warning( "target file is {$target}" );

      $r = error_get_last();
      if ( $r['message'] )
         $this->warning( "filesystem error message: {$r['message']}" );

      $r = $this->app->tar->last_error();
      if ( $r )
         $this->warning( "compression library error message: {$r}" );

      $this->fatal( $msg );
   }



   function save_last_backup_name()
   {
      // get the local file names for the current & prev backup info
      $last_backup_name = $this->app->sys_hostname . EXT_BACKUP_LAST;
      $prev_backup_name = $this->app->sys_hostname . EXT_BACKUP_PREV;

      $last_backup_fn   = dir_fn( $this->path_backup, $last_backup_name );
      $prev_backup_fn   = dir_fn( $this->path_backup, $prev_backup_name );

      // check if a backup was made before the current backup
      if ( file_exists( $last_backup_fn ) )
      {
         // Yes, so was it a failed version of this one 
         //      or was it as previous successful run?
         $prev_backupset_name = trim(file_get_contents( $last_backup_fn ));
         if ( $prev_backupset_name != $this->current_backup )
         {
            // it was a previous successful run, so keep it as the prev. backup
            if ( file_exists( $prev_backup_fn ) )
               if ( unlink( $prev_backup_fn ) === false )
                  $this->fatal( "unable to unlink prev. backup file: {$prev_backup_fn}" );

            if ( rename( $last_backup_fn, $prev_backup_fn ) === false )
               $this->fatal( "unable to rename last backup file: {$last_backup_fn} to {$prev_backup_name}" );
         }
      }

      // keep track of the backup that was just completed
      if ( file_put_contents( $last_backup_fn, $this->current_backup ) === false )
         $this->fatal( "unable to write last backup name ({$this->current_backup}) "
                     . "to {$last_backup_fn}" );

      if ( chmod( $last_backup_fn, $this->app->sys_umaskfiles ) === false )
         $this->fatal( "unable to change mode on file: {$last_backup_fn}" );

      // no problems
      return false;
   }



   function _vfs_hardlink( $linktarget, $linksource )
   {
      if ( file_exists( $linksource ) )
         if ( unlink( $linksource ) === false )
            $this->fatal( "unable to remove expired file {$linksource}" );

      if ( $this->app->verbose )
      {
         $s = str_replace( $this->app->sys_pathbackups, "", $linksource );
         $t = str_replace( $this->app->sys_pathbackups, "", $linktarget );

         $this->tee( "hardlinking {$s} to {$t}" );
      }

      if ( link( $linktarget, $linksource ) === false )
      {
         // something seriously wrong here... multiple partitions?
         $this->fatal( "unable to link {$linksource} to {$linktarget}" );
      }

      return false;
   }


   function _local_addfile( $type, $target, $source )
   {
      $rel_ref = dir_fn( $this->app->current_subfolder . "/" . $target );
      $abs_ref = dir_fn( $this->current_path . "/" . $rel_ref );

      if ( file_exists( $abs_ref ) )
         if ( unlink( $abs_ref ) === false )
            $this->fatal( "unable to remove expired file {$abs_ref}" );

      if ( rename( $source, $abs_ref ) === false )
         $this->_fatal_file_error( "unable to rename/move", $source, $abs_ref );

      if ( chmod( $abs_ref, $this->app->sys_umaskfiles ) === false )
         $this->fatal( "unable to change mode on file: {$abs_ref}" );

      $sha1    = sha1_file( $abs_ref );
      $size    = filesize(  $abs_ref );

      return $this->_vfs_addfile( $target, $sha1, $size );
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

      $this->write_hashlist( dir_fn( $this->current_path, $hash_fn ), $this->current_hashlist   );

      if ( $this->app->verbose )
         $this->tee( "writing hardlinked file list {$hard_fn}" );

      $this->write_hashlist( dir_fn( $this->current_path, $hard_fn ), $this->current_hardlinked );

      if ( $this->app->verbose )
         $this->tee( "writing last/prev backup names" );

      $this->save_last_backup_name();

      // always returns no error result
      return false; 
   }


   function vfs_addfile( $target, $source )
   {
      return $this->_local_addfile( "file", $target, $source );
   }


   function vfs_compress_string( $target, $source )
   {
      $tmp_file = tempnam( $this->path_temp, "sysbackup-" );

      $this->app->tar->last_exec_result(true); // reset the exec result
      if ( $this->app->tar->str( $tmp_file, $source ) != false )
         $this->_fatal_file_error( "unable to compress string", false, $tmp_file );

      return $this->_local_addfile( "file", $target, $tmp_file );
   }


   function vfs_compress_command( $cmd, $target )
   {
      $tmp_file = tempnam( $this->path_temp, "sysbackup-" );

      $this->app->tar->last_exec_result(true); // reset the exec result
      if ( $this->app->tar->command( $cmd, $tmp_file ) != false )
         $this->_fatal_file_error( "unable to get+compress command output", false, $cmd );

      return $this->_local_addfile( "file", $target, $tmp_file );
   }


   function vfs_compress_file( $target, $source )
   {
      $tmp_file = tempnam( $this->path_temp, "sysbackup-" );

      $this->app->tar->last_exec_result(true); // reset the exec result
      if ( $this->app->tar->file( $tmp_file, $source ) != false )
         $this->_fatal_file_error( "unable to compress file", $source, $tmp_file );

      return $this->_local_addfile( "file", $target, $tmp_file );
   }


   function vfs_tarball_folder( $target, $source, $options = array() )
   {
      $tmp_file = tempnam( $this->path_temp, "sysbackup-" );

      $this->app->tar->last_exec_result(true); // reset the exec result
      if ( $this->app->tar->folder( $tmp_file, $source, $options ) != false )
      {
         $msg = $this->app->tar->last_error();
         if ( strpos( $msg, SYS_TAR_FILECHANGED_TEXT ) )
         {
            $this->app->warning( "tarball may contain errors: {$target}" );
            $this->app->warning( "error message was: {$msg}" );
         }
           else $this->_fatal_file_error( "unable to tarball folder", $source, $tmp_file );
      }

      return $this->_local_addfile( "folder", $target, $tmp_file );
   }
}



