<?php // -*- mode: php; mode: mmm; coding: utf-8-unix; -*-
/** 
 *
 * Virtual Backup Filesystem plugin - base class
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



class backupvfs_baseio_c extends plugin_base_c
{
   var $hostname;                 // name of the target host for this object
   var $keeplocal;                // true if a full backup is being saved to this filesystem
                                  // false if the tarballs are created, copied elsewhere, 
                                  // then deleted from this filesystem (if it's local.)

   var $path_temp;                // path to the temp. workspace on the remote system
   var $path_backup;              // path to the base backupset folder on the remote system
   var $path_rotation;            // path to the base rotation storage folder on the remote system

   var $prev_backup;              // Name of the most recent backupset (before the current backup)
   var $prev_path;                // path to the previous backupset
   var $prev_hashlist;            // 
   var $prev_hashlist_rev;        // 
   var $prev_sizelist;            // 
   var $prev_hardlinked;          // 

   var $current_backup;           // Name of the current backupset. Overwrites if already found.
   var $current_path;             // path to the current backupset
   var $current_hashlist;         // 
   var $current_hashlist_rev;     // 
   var $current_sizelist;         // 
   var $current_hardlinked;       // 

   var $current_start;            // stats for the current backup
   var $current_tarball_count;    // 
   var $current_tarball_size;     // 
   var $current_hardlinked_count; //
   var $current_hardlinked_size;  // 

   var $step_start;               // stats for the current step in the overall backup
   var $step_tarball_count;       //
   var $step_tarball_size;        //
   var $step_hardlinked_count;    //
   var $step_hardlinked_size;     //

   var $job_start;                // stats for the current job in the overall backup
   var $job_tarball_count;        //
   var $job_tarball_size;         //
   var $job_hardlinked_count;     //
   var $job_hardlinked_size;      //


   function __construct( &$application_object )
   {
      parent::__construct( $application_object );

      $this->plugin_name              = "vfswriter";
      $this->plugin_title             = "VFS Writer";
      $this->plugin_version           = "0.00a";
      $this->plugin_description       = "";
      $this->plugin_enabled           = false;
      $this->plugin_unique            = false;

      $this->app                      = &$application_object;
      $this->keeplocal                = false;
      $this->hostname                 = "";

      $this->path_temp                = "";
      $this->path_backup              = "";  // full path to the backup root folder
      $this->path_rotation            = "";

      $this->prev_backup              = "";  // name of the prev. backup, ie: dev.azer.ca-20111120
      $this->prev_path                = "";
      $this->prev_hashlist            = array();
      $this->prev_hashlist_rev        = array();
      $this->prev_sizelist            = array();
      $this->prev_hardlinked          = array();

      $this->current_backup           = "";  // name of the current backup, ie: dev.azer.ca-20111123
      $this->current_path             = "";  // full path to the current backup

      $this->current_start            = microtime(true);
      $this->current_tarball_count    = 0;
      $this->current_tarball_size     = 0;
      $this->current_hardlinked_count = 0;
      $this->current_hardlinked_size  = 0;

      $this->step_start               = microtime(true);
      $this->step_tarball_count       = 0;
      $this->step_tarball_size        = 0;
      $this->step_hardlinked_count    = 0;
      $this->step_hardlinked_size     = 0;

      $this->job_start                = microtime(true);
      $this->job_tarball_count        = 0;
      $this->job_tarball_size         = 0;
      $this->job_hardlinked_count     = 0;
      $this->job_hardlinked_size      = 0;

      $this->current_hashlist         = array();
      $this->current_hashlist_rev     = array();
      $this->current_hardlinked       = array();
      $this->current_sizelist         = array();
   }


   function tee( $msg, $eol = "\n" )
   {
      $this->app->tee( "on " . $this->plugin_shortref() . ": " . $msg, $eol );
   }


   function warning( $msg )
   {
      $this->app->warning( "on " . $this->plugin_shortref() . ": " . $msg );
   }


   function fatal( $msg, $stop_flag = true )
   {
      $this->app->fatal( "on " . $this->plugin_shortref() . ": " . $msg, $stop_flag );
   }


   function all_reset()
   {
      $this->step_reset();
      $this->job_reset();

      $this->current_start               = microtime(true);
      $this->current_tarball_count       = 0;
      $this->current_tarball_size        = 0;
      $this->current_hardlinked_count    = 0;
      $this->current_hardlinked_size     = 0;
   }


   function step_reset()
   {
      $this->step_start               = microtime(true);
      $this->step_tarball_count       = 0;
      $this->step_tarball_size        = 0;
      $this->step_hardlinked_count    = 0;
      $this->step_hardlinked_size     = 0;
   }


   function job_reset()
   {
      $this->job_start               = microtime(true);
      $this->job_tarball_count       = 0;
      $this->job_tarball_size        = 0;
      $this->job_hardlinked_count    = 0;
      $this->job_hardlinked_size     = 0;
   }


   function get_stats()
   {
      return array( "current_tarball_count"    => $this->current_tarball_count,
                    "current_tarball_size"     => $this->current_tarball_size,
                    "current_hardlinked_count" => $this->current_hardlinked_count,
                    "current_hardlinked_size"  => $this->current_hardlinked_size,

                    "step_start"               => $this->step_start,
                    "step_tarball_count"       => $this->step_tarball_count,
                    "step_tarball_size"        => $this->step_tarball_size,
                    "step_hardlinked_count"    => $this->step_hardlinked_count,
                    "step_hardlinked_size"     => $this->step_hardlinked_size,

                    "job_start"                => $this->job_start,
                    "job_tarball_count"        => $this->job_tarball_count,
                    "job_tarball_size"         => $this->job_tarball_size,
                    "job_hardlinked_count"     => $this->job_hardlinked_count,
                    "job_hardlinked_size"      => $this->job_hardlinked_size,
                   );
   }


   function stat_str( $type )
   {
      static $myvars = array( "start", 
                              "tarball_count",    "tarball_size", 
                              "hardlinked_count", "hardlinked_size" );

      foreach ( $myvars as $v )
      {
         $n = "{$type}_{$v}";
         $$v = $this->$n;
      }

      $total_time = microtime(true) - $start;
      $bph        = si_suffix(($tarball_size / $total_time) * 60 * 60, "B/h");

      $result     = $this->app->duration( $total_time ) . "s "
                  ;

      if ( ($tarball_count != $hardlinked_count)
        && ($tarball_size  != $hardlinked_size ) )
         $result  = $result
                  . si_suffix( $tarball_size, "B" ) . "/" . $tarball_count
                  ;

      if ( $hardlinked_count )
         $result  = $result
                  . " *"
                  . si_suffix( $hardlinked_size, "B" ) . "/" . $hardlinked_count 
                  . ""
                  ;

      if ( $type == "current" )
         $result  = $result
                  . " (~" . $bph . ")"
                  ;

      return $result;
   }



   function dump_config()
   {
      $this->app->tee( $this->plugin_title );

      $req = array( "hostname"      => array( 3, false, false, "Name of the target host for this object" ),
                    "keeplocal"     => array( 3, false, false, "Flag true if saving a full backup on this filesystem" ),
                    "path_temp"     => array( 3, false, false, "Path to the temp work space" ),
                    "path_backup"   => array( 3, false, false, "Path to the backupsets on this filesystem" ),
                    "path_rotation" => array( 3, false, false, "Path to any backupset rotations on this filesystem" ),
                    );

      $this->app->named_parameters->__dump_properties( $this, $req );
      $this->app->tee("");
   }


   function plugin_help()  
   {
      $this->app->tee( "(help for this plugin is not available yet)" );
   }


   /**
    *
    * Standard Warning
    *
    * Programmers helper: displays a message for methods that have not
    * been defined in the plugin code. In this case it also causes a
    * fatal error if a VFS plugin is loaded via the regular plugin
    * mechanism (not supported for now.)
    *
    */
   function _std_plugin_method_warning()
   {
      $this->app->warning( "This is a VFS plugin. It is not designed to be called in this way." );
      $this->app->fatal( "Invalid call" );
   }


   /**
    *
    * Standard Warning
    *
    * Programmers helper: displays a message for methods that have not
    * been defined in the plugin code
    *
    */
   function _std_vfsplugin_method_warning( $op )
   {
      $this->app->warning( "from plugin {$this->plugin_name}: {$op}" );
   }


   function vfs_init( $backupset_name  = false )
   {
      $this->_std_vfsplugin_method_warning( "dummy vfs init operation" );
   }


   function vfs_start()
   {
      $this->_std_vfsplugin_method_warning( "dummy vfs start operation" );
   }


   function vfs_end()
   {
      $this->_std_vfsplugin_method_warning( "dummy vfs end operation" );
   }


   function vfs_subfolder( $folder_name )
   {
      $this->_std_vfsplugin_method_warning( "dummy vfs subfolder operation" );
   }


   function vfs_addfile( $source_file )
   {
      $this->_std_vfsplugin_method_warning( "dummy vfs addfile operation" );
   }


   function vfs_compress_string( $target_filename, $source_str )
   {
      $this->_std_vfsplugin_method_warning( "dummy vfs compress string operation" );
   }


   function vfs_compress_file( $target, $source )
   {
      $this->_std_vfsplugin_method_warning( "dummy vfs compress file operation" );
   }


   function vfs_tarball_folder( $target, $source )
   {
      $this->_std_vfsplugin_method_warning( "dummy vfs tarball folder operation" );
   }


   function write_hashlist( $hashfn, $hashlist )
   {
      if ( ($fp = fopen( $hashfn, "w" )) === false )
         $this->app->fatal("unable to open the hashlist file for writing");

      if ( is_array( $hashlist ) && count( $hashlist ) )
         foreach ( $hashlist as $fn => $sha1 )
         {
            $out = str_replace( "=", "%3d", $fn )
                 . "="
                 . $sha1
                 . "\n"
                 ;

            if ( fwrite( $fp, $out ) === false )
               $this->app->fatal("unable to write to the hash file {$hashfn}");
         }

      if ( fclose( $fp ) === false )
         $this->app->fatal("unable to close the hashlist file");

      if ( chmod( $hashfn, $this->app->sys_umaskfiles ) === false )
         $this->app->fatal( "unable to change mode on file: {$hashfn}" );
   }


   function read_hashlist( $fn )
   {
      if ( !is_file( $fn ) )
         $this->fatal( "unable to find {$fn}" );

      $text = explode( "\n", file_get_contents( $fn ) );

      $hashlist = array();
      foreach ( $text as $x )
      {
         if ( !$x )
            continue;

         $x = explode( "=", $x );
         $hashlist[str_replace( "%3d", "=", $x[0] )] = $x[1];
      }

      return $hashlist;
   }


   function reverse_hashlist( $hashlist )
   {
      $hashlist_rev = array();
      foreach ( $hashlist as $fn => $sha1 )
      {
        /**
         * more than one file might have the same hash but we only
         * care about one of them (any one) because we assume that
         * hardlinks will always work fine in the backupset folder
         * (It's the same partition for all files in that folder.)
         *
         * NOTE that this can fail miserably if rotations are saved to
         * a different partition and the system writes a symlink to a
         * previous backupset. This is an error. It's better to turn
         * off the linking feature completely in the case where
         * multiple partitions are being used.
         */
         if ( isset($hashlist_rev[$sha1]) )
            continue;

         $hashlist_rev[$sha1] = $fn;
      }

      return $hashlist_rev;
   }



   function _get_last_hashlist( $fn )
   {
      $this->_std_vfsplugin_method_warning( "dummy vfs get_last_hashlist operation" );
   }



   function get_last_hashlist()
   {
      // don't bother to look unless we're using hardlinks
      if ( !$this->_vfs_keeplocal()
        || !$this->_vfs_usehardlinks()
         )
         return false; // not using hardlinks - no need for hashlists

      if ( $this->_get_last_hashlist( $this->app->sys_hostname . EXT_BACKUP_LAST ) )
         return true;

      if ( $this->_get_last_hashlist( $this->app->sys_hostname . EXT_BACKUP_PREV ) )
         return true;

      // no problems
      return false;
   }


   function _folder_check( $folders )
   {
      /**
       * classes that extend the VFS base should check the folders and
       * available space on their file systems and throw an exception
       * if there is a problem.
       *
       */
      $this->_std_vfsplugin_method_warning( "dummy vfs folder check operation" );
   }


   function _vfs_folder_check( $backupset_name )
   {
      // sanity check
      // make sure the local backup folders exist and are writable
      $folders = array( 0 => array( $this->path_temp,                // folder to verify
                                    "temp working",                  // folder name
                                    $this->app->sys_minfreespace,    // min space req'd
                                    true ),                          // check the free space

                        1 => array( $this->path_backup,              // folder to verify
                                    "backup",                        // folder name
                                    $this->app->sys_minbackupsize,   // min space req'd
                                    $this->keeplocal ),              // check the free space

                        2 => array( $this->path_rotation,            // folder to verify
                                    "rotations",                     // folder name
                                    $this->app->sys_minbackupsize,   // min space req'd
                                    false ),                         // check the free space
                      );


      $this->current_backup = $backupset_name;
      $this->current_path = dir_fn($this->path_backup . "/" . $backupset_name);

      $this->_folder_check( $folders );
   }



   function _vfs_hardlink( $linktarget, $linksource )
   {
      /**
       * classes that extend the VFS base should create a hardlink at
       * $linksource that links to $linktarget. It is acceptible to
       * throw an exception if the hardlink fails; otherwise return
       * true for error and the caller will throw an exception.
       *
       */

      $this->_std_vfsplugin_method_warning( "dummy vfs hardlink operation" );
   }


   function _vfs_keeplocal()
   {
      $this->_std_vfsplugin_method_warning( "dummy vfs return keeplocal flag" );
   }


   function _vfs_usehardlinks()
   {
      $this->_std_vfsplugin_method_warning( "dummy vfs return usehardlinks flag" );
   }


   function _vfs_addfile( $target, $sha1, $size )
   {
      $rel_ref = dir_fn( $this->app->current_subfolder . "/" . $target );
      $abs_ref = dir_fn( $this->current_path . "/" . $rel_ref );

      $this->current_tarball_count     += 1;
      $this->current_tarball_size      += $size;

      $this->step_tarball_count        += 1;
      $this->step_tarball_size         += $size;

      $this->job_tarball_count         += 1;
      $this->job_tarball_size          += $size;

      $this->current_sizelist[$rel_ref] = $size;
      $this->current_hashlist[$rel_ref] = $sha1;
      
      // don't try to update the reverse hashlist here because
      // we need to query it in the next step

      /*
       * check for identical tarballs in a previous backup set
       */
      if ( $this->_vfs_keeplocal()
        && $this->_vfs_usehardlinks()
        && isset($this->prev_hashlist_rev[$sha1]) )
      {
         /*
          * Okay, so we've just created a tarball that's identical to
          * a tarball that was created as part of the last run. Since
          * we're working here on the daily backups we can assume
          * (sorry - but, more often than not it's true,) we can
          * assume that the filesystem on which the previous backup
          * was written is the same as the filesystem for the current
          * local backup. In this case we can go ahead and remove the
          * new tarball and hardlink to the previous one.
          */
         $linktarget = dir_fn( $this->prev_path, $this->prev_hashlist_rev[$sha1] );
         $linksource = $abs_ref;

         if ( ($this->_vfs_hardlink( $linktarget, $linksource )) !== false )
            $this->app->fatal( "unable to link {$linksource} to {$linktarget}" );

         /*
          * keep track of the fact that a hardlink was created so that
          * a future utility can scan the hardlinked files to
          * determine if any of them have experienced a disk
          * error. This is because it's possible that the same tarball
          * will be hardlinked over and over for years.
          */
         $this->current_hardlinked_count    += 1;
         $this->current_hardlinked_size     += $size;
         $this->current_hardlinked[$rel_ref] = $sha1;

         $this->step_hardlinked_count       += 1;
         $this->step_hardlinked_size        += $size;

         $this->job_hardlinked_count        += 1;
         $this->job_hardlinked_size         += $size;
      }
        else /**
              * check for any identical tarballs in the current backup set
              *
              */
             if ( isset( $this->current_hashlist_rev[$sha1] ) )
             {
                /**
                 * Okay, so we've just created a tarball that's identical to
                 * a different tarball. In this case we can go ahead and remove the
                 * new tarball and hardlink to the existing one.
                 */
                $linktarget = dir_fn( $this->current_path, $this->current_hashlist_rev[$sha1] );
                $linksource = $abs_ref;

                if ( ($this->_vfs_hardlink( $linktarget, $linksource )) !== false )
                   $this->app->fatal( "unable to link {$linksource} to {$linktarget}" );

                /*
                 * keep track of the fact that a hardlink was created so that
                 * a future utility can scan the hardlinked files to
                 * determine if any of them have experienced a disk
                 * error. This is because it's possible that the same tarball
                 * will be hardlinked over and over for years.
                 */
                $this->current_hardlinked_count    += 1;
                $this->current_hardlinked_size     += $size;
                $this->current_hardlinked[$rel_ref] = $sha1;

                $this->step_hardlinked_count       += 1;
                $this->step_hardlinked_size        += $size;

                $this->job_hardlinked_count        += 1;
                $this->job_hardlinked_size         += $size;
             }

      // remember to keep track of the new tarball
      if ( !isset( $this->current_hashlist_rev[$sha1] ) )
         $this->current_hashlist_rev[$sha1] = $rel_ref;

      return false;
   }
}




