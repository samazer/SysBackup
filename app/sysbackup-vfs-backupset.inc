<?php // -*- mode: php; mode: mmm; coding: utf-8-unix; -*-
/** 
 *
 * Virtual Backup Filesystem Generic Backupset Code
 *
 * As I implement the rotations modules it becomes clear that the
 * backupsets (and the hashlists) are complex objects that need to be
 * factored out of the existing code so that they can be re-used.
 *
 * Basically the process of writing the backups is not much different
 * from the case where a rotation is being copied to a separate
 * partition. Hardlinks don't work in this case so it's necessary to
 * check the previously written rotation folder to see if there are
 * any identical files that can be hardlinked on the other
 * partition. The process is essentially the same as the process for
 * registering a tarball with the main backupset during a backup.
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
 * The Backupset Information Class
 *
 * This class attempts to carry all the info associated with a
 * backupset, ie: the current hashlist for the backupset being created
 * and the hashlist for any previous backupset in the associated
 * rotation.
 *
 * Actual file operations will be managed separately.
 *
 */
class backupvfs_backupset_c extends plugin_appreference_c
{
   var $vfs;              // a reference to the VFS object. Used for file I/O

   var $current_hashlist; // hashlist for the backupset being written

   var $prev_hashlist;    // hashlist for the previous written backupset in the same partition
   var $prev_path;        // the path to the backupset from which the above hashlist was loaded



   function plugin_shortref()
   {
      return "backupset@" . $this->vfs->plugin_shortref();
   }



  /**
   *
   * Get the Previous Path
   *
   * If this class finds a suitable hashlist for a previous backupset
   * it records the path to that backupset in the prev_path
   * property. Later during the backup it might be determined that it
   * is possible to create a hardlink to one of the files in that
   * backpset - so code will use this method to get the path to the
   * base of that backupset.
   *
   */
   function get_prev_path() 
   { 
      return $this->prev_path; 
   }



   function __construct( &$application_object, &$vfs_object )
   {
      parent::__construct( $application_object );

      $this->vfs              = &$vfs_object;

      $this->current_hashlist = new backupvfs_hashlist_c( $vfs_object );
      $this->prev_hashlist    = new backupvfs_hashlist_c( $vfs_object );
   }


   function job_reset()    { $this->current_hashlist->job_reset();  }
   function step_reset()   { $this->current_hashlist->step_reset(); }

   function get_stats()    { return $this->current_hashlist->get_stats();  }

   function filesize( $rel_ref )      { return $this->current_hashlist->filesize( $rel_ref ); }
   function filehash( $rel_ref )      { return $this->current_hashlist->filehash( $rel_ref ); }
   function is_hardlinked( $rel_ref ) { return $this->current_hashlist->is_hardlinked( $rel_ref ); }


   /**
    *
    * Get Last Hashlist
    *
    * For the local hashlist we just need to check the backup root
    * path for the name/path of the last backup (which is in a file
    * called FN_PATH_BACKUP_LAST.) If the backupset name for that file is the
    * same as the date for today we need to ignore it.
    *
    */
   function _get_last_hashlist( $folder, $last_name, $hashlist_name )
   {
      $last_backup = dir_fn( $folder, $last_name );

      if ( $this->app->verbose() )
         $this->tee( "looking for hashlist in {$last_backup}" );

      if ( $this->vfs->_vfs_file_exists( $last_backup ) )
      {
         $last_backup_name = trim( $this->vfs->_vfs_file_get_contents( $last_backup ) );

         if ( $this->app->verbose() )
         {
            $this->tee( "Current Backup Name is " . $this->app->get_backupset_name() );
            $this->tee( "Last Backup Name is {$last_backup_name}" );
         }

         if ( $last_backup_name != $this->app->get_backupset_name() )
         {
            $fn = dir_fn( $folder . "/" . $last_backup_name, $hashlist_name );

            if ( $this->app->verbose() )
               $this->tee( "looking for hashlist {$fn}" );

            if ( $this->vfs->_vfs_file_exists( $fn ) )
            {
               $this->prev_path = dir_fn( $folder, $last_backup_name );
               $this->prev_hashlist->read( $fn );

               if ( $this->app->verbose() )
                  $this->tee( "got hashlist {$fn}" );

               $this->app->log( "loading hashlist " . $this->vfs->vfs_displayref($fn) );

               return true;
            } 
              else 
            {
               $this->warning( "hashlist not found in " . $this->vfs->vfs_displayref($fn) );
               return false;
            }
         }
      }

      return false;
   }


   /**
    * 
    * Get the hashlist from the previous successful backup run.
    *
    * When the backup software starts running it checks the storage
    * device to find the name of the last backupset that was
    * successfully generated. If that backupset has the same name as
    * the backupset that is being generated as part of the current run
    * then that backupset must be over-written. If that backupset was
    * generated on a previous day then it is possible to save a great
    * deal of disk space by hardlinking files in cases where the
    * previous copy is identical to the new copy.
    *
    * This function searches the specified folder to determine which
    * previously written backupset can be used for this purpose. Note
    * that there is no need for this operation in cases where the VFS
    * is configured to not keep previous backupset. It is also not
    * needed in cases where hardlinks are not being used (either
    * because they are disabled or because they don't work.)
    *
    */
   function get_last_hashlist( $folder )
   {
      // nothing to look for?
      if ( !$this->vfs->get_keeplocal() )
         return false;

      // hardlinks not allowed?
      if ( !$this->vfs->get_usehardlinks() )
         return false;

      if ( $this->_get_last_hashlist(   $folder,         
                                        $this->app->get_hostname() . EXT_BACKUP_LAST, 
                                        $this->app->get_hostname() . EXT_HASHLIST
                                        ) )
         return true;

      return $this->_get_last_hashlist( $folder,
                                        $this->app->get_hostname() . EXT_BACKUP_PREV, 
                                        $this->app->get_hostname() . EXT_HASHLIST
                                      );
   }


   /**
    *
    * Save Last Backup Name
    *
    * After each backupset has been written it is necessary to update
    * the key files in the root folder for the rotation. 
    *
    * These files contain only the most recent backupset names.
    *
    * The "last backupset" is written to a file called <hostname>.last
    * and the name of the backupset before that is written to a file
    * called <hostname>.prev.
    *
    */
   function save_last_backup_name( $folder )
   {
      // get the local file names for the current & prev backup info
      $last_backup_name = $this->app->get_hostname() . EXT_BACKUP_LAST;
      $prev_backup_name = $this->app->get_hostname() . EXT_BACKUP_PREV;

      $last_backup_fn   = dir_fn( $folder, $last_backup_name );
      $prev_backup_fn   = dir_fn( $folder, $prev_backup_name );

      // check if a backup was made before the current backup
      if ( $this->vfs->_vfs_file_exists( $last_backup_fn ) )
      {
         // Yes, so was it a failed version of this one 
         //      or was it as previous successful run?
         $prev_backupset_name = trim($this->vfs->_vfs_file_get_contents( $last_backup_fn ));
         if ( $prev_backupset_name != $this->app->get_backupset_name() )
         {
            // it was a previous successful run, so keep it as the prev. backup
            if ( $this->vfs->_vfs_file_exists( $prev_backup_fn ) )
               if ( $this->vfs->_vfs_unlink( $prev_backup_fn ) === false )
                  $this->fatal( "unable to unlink prev. backup file: {$prev_backup_fn}" );

            if ( $this->vfs->_vfs_rename( $last_backup_fn, $prev_backup_fn ) === false )
               $this->fatal( "unable to rename last backup file: {$last_backup_fn} to {$prev_backup_name}" );
         }
      }

      // keep track of the backup that was just completed
      if ( $this->vfs->_vfs_file_put_contents( $last_backup_fn, $this->vfs->get_backupset_name() ) === false )
         $this->fatal( "unable to write last backup name to {$last_backup_fn}" );

      if ( $this->vfs->_vfs_chmod( $last_backup_fn, $this->vfs->get_umaskfiles() ) === false )
         $this->fatal( "unable to change mode on file: {$last_backup_fn}" );

      // no problems
      return false;
   }


   /**
    *
    * should_hardlink( $rel_ref, $sha1 )
    *
    * Return false if the file should simply be copied into the backupset.
    * Return 1 if the file should be hardlinked to an identical file in the current backupset.
    * Return 2 if the file should be hardlinked to an identical file in the previous backupset.
    *
    */
   function should_hardlink( $sha1 )
   {
      if ( !$this->vfs->get_keeplocal() )
         return false; // not applicable as we're not saving local copies

      if ( !$this->vfs->get_usehardlinks() )
         return false; // not applicable as we're not using hardlinks

      if ( $this->current_hashlist->hash_exists( $sha1 ) )
         return 1;

      if ( $this->prev_hashlist->hash_exists( $sha1 ) )
         return 2;

      // not found - so copy the file into the backupset
      return 0;
   }


   /**
    *
    * _Register a tarball with the current backupset
    *
    * See the notes for the register() method below.
    *
    * The _register() method below takes a $where parameter to
    * determine where to hardlink a file. It also updates the various
    * information arrays and stats.
    *
    */
   function _register( $where, $rel_ref, $sha1, $size )
   {
      // can we hardlink to a file in the current backupset?
      switch ( $where )
      {
         case 0:
            /**
             *
             * Register the data for a tarball that has (already) been
             * added to the current backupset.
             *
             * Updates the necessary arrays for the current backupset with the
             * specified data.
             *
             */
            $this->current_hashlist->register( $rel_ref, $sha1, $size, false );
            return $where;


         case 1:
            /**
             * Okay, so we've got a tarball that's identical to to a
             * different tarball in the current backupset. In this
             * case we can go ahead and hardlink to the existing one.
             */
             $linktarget = dir_fn( $this->vfs->get_path_backupset(), 
                                   $this->current_hashlist->fileref($sha1) );

             $linksource = dir_fn( $this->vfs->get_path_backupset(), $rel_ref );

             if ( ($this->vfs->vfs_hardlink( $linktarget, $linksource )) !== false )
                $this->fatal( "unable to link " . $this->vfs->vfs_displayref( $linksource ) 
                            . " to " . $this->vfs->vfs_displayref( $linktarget ) );

            /**
             *
             * Register the data for a tarball that has been added to
             * the current backupset.
             *
             * Updates the necessary arrays for the current backupset
             * with the specified data.
             *
             */
             $this->current_hashlist->register( $rel_ref, $sha1, $size, true );
             return $where;


         case 2:
            /*
             * Okay, so we've created a tarball that's identical to
             * a tarball that was created as part of the last run.  In
             * this case we can go ahead and remove the new tarball and
             * hardlink to the previous one.
             */
            $linktarget = dir_fn( $this->get_prev_path(), 
                                  $this->prev_hashlist->fileref($sha1) );

            $linksource = dir_fn( $this->vfs->get_path_backupset(), $rel_ref );

            if ( ($this->vfs->vfs_hardlink( $linktarget, $linksource )) !== false )
                $this->fatal( "unable to link " . $this->vfs->vfs_displayref( $linksource ) 
                            . " to " . $this->vfs->vfs_displayref( $linktarget ) );

            /**
             *
             * Register the data for a tarball that has been added to the current backupset.
             *
             * Updates the necessary arrays for the current backupset with the
             * specified data.
             *
             */
            $this->current_hashlist->register( $rel_ref, $sha1, $size, true );
            return $where;


         default:
            break;
      }

      $this->fatal( "unknown destination code" );
   }


   /**
    *
    * Register a tarball with the current backupset
    *
    * This is the method that does most of the work involved in
    * registering a tarball with the current backupset. The method
    * assumes that the tarball has already been written to the
    * destination folder on the virtual file system - so there is no
    * need to copy files. The idea here is to try to save disk space
    * by taking advantage of other identical files that might have
    * been written before to the current or previous backupsets.
    *
    * The _register() method above takes a $where parameter to
    * determine where to hardlink a file. The register() method below
    * determines the value of the $where parameter.
    *
    */
   function register( $rel_ref, $sha1, $size )
   {
      // find out if the file should be hardlinked somewhere?
      if ( ($where = $this->should_hardlink( $sha1 )) == false )
      {
         /**
          *
          * No, just register the data for a tarball that has been
          * added to the current backupset.
          *
          * Updates the necessary arrays for the current backupset
          * with the specified data.
          *
          */
         $this->current_hashlist->register( $rel_ref, $sha1, $size, false );
         return $where;
      }

      /**
       *
       * Yes, so delete the copy and write a hardlink to a previous
       * copy.
       *
       * Also updates the necessary arrays for the current backupset
       * with the specified data.
       *
       */
      return $this->_register( $where, $rel_ref, $sha1, $size );
   }



  /**
   *
   * Add File
   *
   * This method that adds a temporary file to the backupset.  The
   * $source parameter must be an absolute reference to the tarball to
   * be copied into the backupset. The $target parameter is the
   * relative name of the file within the backupset. This generally
   * includes the subfolder name, a slash and the target file name
   * which must be encoded as per the convention for backupset files.
   *
   * In general the VFS will create a tarball on a local filesystem,
   * get the size of the tarball and calculate the hash on the local
   * filesystem, then copy the file to the remote filesystem before
   * registering it with the backupset arrays. Obviously any VFS that
   * must do things differently will need to either override this
   * method or skip it and call the register() method when the file
   * has been properly added to the backupset. 
   *
   * In practice this method will return the $where result of the
   * register() method. It will throw an error of something goes
   * wrong.
   *
   */
   function addfile( $target, $source_abs_ref )
   {
      $abs_ref = dir_fn( $this->vfs->get_path_backupset(), $target );

      /**
       * 
       * In cases where the current backupset is being re-tried after
       * a failed attempt there will be existing files in the
       * backupset folder that need to be removed before the current
       * files can be installed there.
       *
       */
      if ( $this->vfs->_vfs_file_exists( $abs_ref ) )
         if ( $this->vfs->_vfs_unlink( $abs_ref ) === false )
            $this->fatal( "unable to remove expired file {$abs_ref}" );

      /**
       *
       * If the temp. file is on the same partition as the daily
       * rotation the following operation will be instantaneous. It
       * will only move/rename the directory entry on the
       * filesystem. However, in cases where the temp. files are on a
       * different partition the rename operation will result in a
       * file move/copy operation which will take some time.
       *
       */
      if ( $this->vfs->_vfs_rename( $source_abs_ref, $abs_ref ) === false )
         $this->vfs->_fatal_file_error( "unable to rename/move", $source_abs_ref, $abs_ref );

      // set the desired file perms on the file in the backupset.
      if ( $this->vfs->_vfs_chmod( $abs_ref, $this->vfs->get_umaskfiles() ) === false )
         $this->fatal( "unable to change mode on file: {$abs_ref}" );

      /**
       *
       * In this last step the file which has now been installed in
       * the backupset must be registered with the various hashlist
       * arrays and stats must be collected. During this next step we
       * might find that an identical file has been written in the
       * past either to the current backupset or to a previous
       * backupset - in which case we'll delete the new (extra) copy
       * and hardlink to the previous copy in order to save space on
       * the filesystem.
       *
       */
      $sha1    = $this->vfs->_vfs_sha1_file( $abs_ref );
      $size    = $this->vfs->_vfs_filesize(  $abs_ref );

      // the following call will die if it fails
      $this->register( $target, $sha1, $size );

      // return OK
      return false;
   }
}


