<?php // -*- mode: php; mode: mmm; coding: utf-8-unix; -*-
/** 
 *
 * Virtual Backup Filesystem I/O code
 *
 * The minimal list of I/O functions that must be overridden by each
 * Virtual File System. The methods in this class can be used to read
 * and write hashlists and last/prev files on the local
 * filesystem. Each additional VFS must override the methods to
 * support other file systems.
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
 * Basic VFS low-level fileio methods
 *
 * These represent a minimal list of file I/O methods that must be
 * overridden for each different type of filesystem that will be used
 * to store backupsets.
 *
 */
class backupvfs_fileio_c extends plugin_base_c
{
  /**
   *
   * Minimal constructor
   *
   * Not much to do here
   *
   */
   function __construct( &$application_object )
   {
      parent::__construct( $application_object );
   }


  /**
   *
   * Fatal File Error
   *
   * This method tries to produce a meaningful error message in the
   * event of an error on the filesystem. This method will likely be
   * overridden by most VFS classes.
   *
   */
   function _vfs_fatal_file_error( $msg, $source = false, $target = false  )
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
      return file_exists( $fn );
   }


   /**
    *
    * return the contents of a small file
    *
    */
   function _vfs_file_get_contents( $fn )
   {
      return file_get_contents( $fn );
   }


   /**
    *
    * unlink/delete a small file
    *
    */
   function _vfs_unlink( $fn )
   {
      return unlink( $fn );
   }


   /**
    *
    * hardlink to an existing file
    *
    */
   function _vfs_link( $target, $source )
   {
      return link( $target, $source );
   }


   /**
    *
    * return the contents of a small file
    *
    */
   function _vfs_rename( $from, $to )
   {
      return rename( $from, $to );
   }


   /**
    *
    * Write text to a small file
    *
    */
   function _vfs_file_put_contents( $fn, $content )
   {
      return file_put_contents( $fn, $content );
   }


   /**
    *
    * Change the permissions of a file
    *
    */
   function _vfs_chmod( $fn, $mask )
   {
      return chmod( $fn, $mask );
   }


   /**
    *
    * Change the permissions of a file
    *
    */
   function _vfs_mkdir( $dir )
   {
      return mkdir( $dir, $this->get_umaskfolders(), true );
   }


   /**
    *
    * determine if write permission is available
    *
    */
   function _vfs_is_writable( $dir )
   {
      return is_writable( $dir );
   }


   /**
    *
    * determine the amount of free space available
    *
    */
   function _vfs_disk_free_space( $dir )
   {
      return disk_free_space( $dir );
   }


   /**
    *
    * return the size of a file
    *
    */
   function _vfs_filesize( $fn )
   {
      return filesize( $fn );
   }


   /**
    *
    * calculate an sha1 hash for a file
    *
    */
   function _vfs_sha1_file( $fn )
   {
      return sha1_file( $fn );
   }


   /**
    *
    * determine the total amount of disk space available
    *
    */
   function _vfs_disk_total_space( $dir )
   {
      return disk_total_space( $dir );
   }


   /**
    *
    * Determine, using brute force, if it is possible to hardlink
    * between these two folders
    *
    */
   function _vfs_can_hardlink( $target_dir, $source_dir )
   {
      return can_hardlink( $target_dir, $source_dir );
   }


   /**
    *
    * List items in a folder that have expired
    *
    */
   function _vfs_list_expired_items( $path, $max )
   {
      return list_expired_items( $path, $max );
   }


   /**
    *
    * Remove items in a folder that have expired
    *
    */
   function _vfs_remove_expired_items( $path, $max, $debug = true, $folders_only = true )
   {
      return remove_expired_items( $path, $max, $debug, $folders_only );
   }


/******************************************************************************
 *
 * fileio methods likely to be used by all virtual file systems
 *
 * These represent a minimal list of file I/O methods that are
 * expected to be generally useful - but can be overridden for each
 * different type of filesystem as needed.
 *
 ******************************************************************************/


  /**
   *
   * Change an absolute file reference so that it is easier to display and read.
   *
   * Looks for substrings representing the <temp>, <daily>, <weekly>
   * and <monthly> folders and replaces them with easy-to-read strings
   * for use in error messages and other displays.
   *
   * TODO: expand this to include remote systems
   * TODO: add the short hostname to the output
   *
   */
   function vfs_displayref( $ref )
   {
      static $all_paths = array();

      if ( !count( $all_paths ) )
         $all_paths = $this->get_paths();

      foreach( $all_paths as $id => $p )
         $ref = str_replace( $p, $id, $ref );

      return $ref;
   }



  /**
   *
   * Mark Backupset incomplete by writing a file called INCOMPLETE to the backupset folder
   *
   */
   function _mark_backupset_incomplete()
   {
      $incomplete_fn = dir_fn( $this->path_backupset, BACKUPVFS_INCOMPLETE_FILENAME );
      if ( $this->_vfs_file_put_contents( $incomplete_fn, BACKUPVFS_INCOMPLETE_TEXT ) === false )
         $this->_vfs_fatal_file_error( "unable to write to a temp file", false, $incomplete_fn );
   }


  /**
   *
   * Unmark the backupset by removing the file called INCOMPLETE from the backupset folder
   *
   */
   function _unmark_backupset_incomplete()
   {
      $incomplete_fn = dir_fn( $this->path_backupset, BACKUPVFS_INCOMPLETE_FILENAME );
      if ( $this->_vfs_unlink( $incomplete_fn ) === false )
         $this->_vfs_fatal_file_error( "unable to unlink a temp file", false, $incomplete_fn );
   }




  /**
   *
   * Folder Check
   *
   * This method is intended to verify that a folder:
   *
   * (1) Exists
   * (2) Is writable
   * (3) Has a specified amount of disk space
   *
   * This method should be called during the configuration of the
   * backup system. It is expected to immediately error out if there
   * is a problem so that the sysadmin can see it and correct it
   * quickly.
   *
   * 
   *
   */
   function vfs_folder_check( $short_name, $folder_path, $min_space_required, $check_space )
   {
      $problems_found = false;
      if ( !$this->_vfs_file_exists( $folder_path ) )
      {
         $problems_found = true;
         $this->warning( "Unable to find the {$short_name} folder at '{$folder_path}'" );
      }
        else
      {
         if ( !$this->_vfs_is_writable( $folder_path ) )
         {
            $problems_found = true;
            $this->warning( "need write permission on the {$short_name} folder at '{$folder_path}'" );
         }

         if ( $check_space )
            if ( ($free_space = $this->_vfs_disk_free_space( $folder_path )) < $min_space_required )
            {
               $problems_found = true;
               $this->warning( "Free space of " . si_suffix( $free_space, "B" ) 
                             . "on the {$short_name} is below the required " 
                             . si_suffix( $min_space_required ) );
            }
      }

      return $problems_found;
   }



   function vfs_warn_disk_space( $full_check = false )
   {
      $problems_found = false;

      if ( $this->minfreespace )
         if ( $this->vfs_folder_check( "temp", $this->path_temp, $this->minfreespace, true ) )
             $problems_found = true;

      if ( $full_check && $this->minbackupsize )
         if ( $this->vfs_folder_check( "daily", $this->path_daily, $this->minbackupsize, true ) )
            $problems_found = true;

      return $problems_found;
   }



  /**
   *
   * Create the backupset
   *
   */
   function vfs_createbackupset_folder( $base )
   {
      $this->current_subfolder        = "";
      $this->path_backupset           = dir_fn( $base, $this->app->get_backupset_name() );
      $this->path_backupset_subfolder = $this->path_backupset;

      if ( $this->_vfs_file_exists($this->path_backupset) == true )
      {
         if ( $this->_vfs_chmod( $this->path_backupset, $this->get_umaskfolders() ) === false )
            $this->fatal( "unable to change mode on subfolder: {$this->path_backupset}" );

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


      if ( $this->_vfs_mkdir( $this->path_backupset, $this->get_umaskfolders(), true ) === false )
         $this->fatal( "unable to create the local backup folder: {$this->path_backupset}" );

      if ( $this->_vfs_chmod( $this->path_backupset, $this->get_umaskfolders() ) === false )
         $this->fatal( "unable to change mode on subfolder: {$this->path_backupset}" );

      return false;
   }


  /**
   *
   * Set and, if necessary, create the specified subfolder within the backupset
   *
   */
   function vfs_subfolder( $folder_name )
   {
      $this->current_subfolder     = $folder_name;
      $this->path_backupset_folder = dir_fn($this->path_backupset . "/" . $folder_name);

      if ( $this->_vfs_file_exists( $this->path_backupset_folder ) )
         return false;

      if ( $this->_vfs_mkdir( $this->path_backupset_folder, $this->get_umaskfolders(), true ) === false )
         $this->fatal( "unable to create subfolder: {$folder_name}" );

      if ( $this->_vfs_chmod( $this->path_backupset_folder, $this->get_umaskfolders() ) === false )
         $this->fatal( "unable to change mode on subfolder: {$subfolder}" );

      return false;
   }






   /**
    *
    * classes that extend the VFS base should create a hardlink at
    * $linksource that links to $linktarget. It is acceptible to
    * throw an exception if the hardlink fails; otherwise return
    * true for error and the caller will throw an exception.
    *
    * Throwing an exception at this level is acceptable because the
    * various flags should be set correctly so that this method should
    * not be called without an appropriate expectation of success.
    *
    * Note that there is a new brute force library function in
    * general-sysutils.inc that is able to determine (by trial and
    * error) if a hardlink will succeed or fail. Once this is
    * implemented in the remote fileio module it will be possible to
    * verify programmatically whenever hardlinks are available.
    *
    */
   function vfs_hardlink( $linktarget, $linksource )
   {
     /**
      * be sure that the source/link doesn't exist already
      */
      if ( $this->_vfs_file_exists( $linksource ) )
         if ( $this->_vfs_unlink( $linksource ) === false )
            $this->fatal( "unable to remove expired file {$linksource}" );

      if ( $this->app->verbose() )
      {
         $s = $this->vfs_displayref( $linksource );
         $t = $this->vfs_displayref( $linktarget );

         $this->tee( "hardlinking {$s} to {$t}" );
      }

      if ( $this->_vfs_link( $linktarget, $linksource ) === false )
      {
         $s = $this->vfs_displayref( $linksource );
         $t = $this->vfs_displayref( $linktarget );

         // something seriously wrong here... multiple partitions?
         $this->fatal( "unable to link {$s} to {$t}" );
      }

      return false;
   }



   function _vfs_prune_expired_backups( $description, $path_to_prune, $max_items )
   {
      if ( ($items_to_prune = $this->_vfs_list_expired_items( $path_to_prune, $max_items )) === false )
      {
         $this->warning( "disk error while scanning " . $this->vfs_displayref( $path_to_prune ) );
         return;
      }

      if ( !count($items_to_prune) )
         return false;

      $this->app->_notquiet_report_header();

      $this->app->tee( "    {$description}:" );

      foreach ( $items_to_prune as $f => $i )
      {
         $name  = $f;
         $stamp = $i[0];
         $size  = $i[1];

         $desc  = date( "Y.m.d h:i", $stamp )
                . " "
                . str_pad( si_suffix( $size ), 8, " ", STR_PAD_LEFT )
                . " "
                . $name
                ;

         $this->app->tee( "      {$desc}" );
      }

      $total  = si_suffix($this->_vfs_disk_total_space( $path_to_prune ), "B");
      $before = $this->_vfs_disk_free_space( $path_to_prune );
      $count  = count( $items_to_prune );

      $this->_vfs_remove_expired_items( $path_to_prune, $max_items, false );

      $after  = $this->_vfs_disk_free_space( $path_to_prune );
      $saved  = si_suffix($after - $before, "B");
      $after  = si_suffix( $after, "B" );
      $this->app->tee( "      --- {$saved}/{$count} released, {$after} of {$total} available" );

      return false;
   }
}



