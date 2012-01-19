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
 * @package sysbackup
 * @subpackage sysbackup
 * @license http://www.gnu.org/licenses/agpl.html AGPL
 * @copyright 2011 by Sam Azer, All rights reserved
 * @link http://www.azertech.net/
 * @link http://www.samazer.net/about
 *
 */



define( "BACKUPVFS_INCOMPLETE_FILENAME", "INCOMPLETE" );
define( "BACKUPVFS_INCOMPLETE_TEXT",     "This backupset is incomplete." );



class backupvfs_baseio_c extends backupvfs_fileio_c
{
   var $hostname;               // long host name used for naming of backup sets
   var $hostid;                 // short host name used for naming of backup sets

   var $named_parameters;
   var $np_default_method;
   var $np_default_index;

   var $path_temp;              // path to the temp. workspace on the virtual file system
   var $path_daily;             // path to the base backupset folder on the virtual file system
   var $path_weekly;            // path to the weekly rotation storage folder on the virtual file system
   var $path_monthly;           // path to the weekly rotation storage folder on the virtual file system

   var $usehardlinks;           // true to try to save disk space by hardlinking identical files
   var $keeplocal;              // true to keep a full copy of the backup on the local filesystem
                                // false if the tarballs are created, copied elsewhere, then deleted from this filesystem

   var $umaskfolders;           // default umask to use for created directories
   var $umaskfiles;             // default umask to use for created files

   var $backupset;              // the backupset object for this VFS
   var $backupset_name;         // name of the current backupset
   var $path_backupset;         // path to the current backupset
   var $path_backupset_folder;  // path to the current folder within the backupset
   var $current_subfolder;      // name of the currently selected subfolder

   var $minbackupsize;          // Minimum free space needed to hold one full backup (if desired)
   var $minfreespace;           // Minimum free space needed for operations. 
                                // This is usually the maximum size of the biggest 
                                // individual tarball in the backup set.

   var $rotation_enabled;       // flag true if rotations are enabled for this vfs
   var $rotation_dayofweek;     // day of the week to rotate daily backups
   var $rotation_dayofmonth;    // day of the month to rotate monthly backups
   var $rotation_total_daily;   // total number of daily backups in the daily rotation
   var $rotation_total_weekly;  // total number of weekly backups in the weekly rotation
   var $rotation_total_monthly; // total number of monthly backups in the monthly rotation



   function set_hostname( $nam, $id )  { $this->hostname = $nam; 
                                         $this->hostid   = $id;                 }

   function get_hostname()             { return $this->hostname;                }
   function get_hostid()               { return $this->hostid;                  } 

   function get_path_temp()            { return $this->path_temp;               }
   function get_path_daily()           { return $this->path_daily;              }
   function get_path_weekly()          { return $this->path_weekly;             }
   function get_path_monthly()         { return $this->path_monthly;            }
   function get_path_backupset()       { return $this->path_backupset;          }
   function get_current_subfolder()    { return $this->current_subfolder;       }
   function get_backupset_name()       { return $this->backupset_name;          }


   function job_reset()    { $this->backupset->job_reset();  }
   function step_reset()   { $this->backupset->step_reset(); }
   function all_reset()    { $this->backupset->job_reset();
                             $this->backupset->step_reset(); }

   function get_stats()    { return $this->backupset->get_stats();  }


   function filesize( $rel_ref )      { return $this->backupset->filesize( $rel_ref ); }
   function filehash( $rel_ref )      { return $this->backupset->filehash( $rel_ref ); }
   function is_hardlinked( $rel_ref ) { return $this->backupset->is_hardlinked( $rel_ref ); }


   function get_paths()
   {
      $id = $this->get_hostid();
      return array( "[{$id}:temp]"    => $this->path_temp,
                    "[{$id}:daily]"   => $this->path_daily,
                    "[{$id}:weekly]"  => $this->path_weekly,
                    "[{$id}:monthly]" => $this->path_monthly,
                    "[{$id}:current]" => $this->path_backupset,
                    "[{$id}:prev]"    => $this->backupset->get_prev_path(),
                  );
   }


   /**
    *
    * Keep Local?
    *
    * Each VFS can be configured to disable local storage of backups.
    *
    */
   function get_keeplocal()            { return $this->keeplocal;               }

   /**
    *
    * Use Hardlinks?
    *
    * Each VFS can be configured to disable the use of HardLinks
    * between backupsets (even between backupsets in the daily
    * rotation.)
    *
    */
   function get_usehardlinks()         { return $this->usehardlinks;            }


   function get_minfreespace()         { return $this->minfreespace;            }
   function get_minbackupsize()        { return $this->minbackupsize;           }
   function get_umaskfolders()         { return $this->umaskfolders;            }
   function get_umaskfiles()           { return $this->umaskfiles;              }


   function get_rotation_enabled()     { return $this->rotation_enabled;        }
   function get_rotation_dayofweek()   { return $this->rotation_dayofweek;      }
   function get_rotation_dayofmonth()  { return $this->rotation_dayofmonth;     }
   function get_rotations_daily()      { return $this->rotation_total_daily;    }
   function get_rotations_weekly()     { return $this->rotation_total_weekly;   }
   function get_rotations_monthly()    { return $this->rotation_total_monthly;  }


   function plugin_shortref()
   {
      return $this->get_hostid();
   }


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
      $this->hostname                 = "";
      $this->hostid                   = "";

      $this->path_temp                = "";
      $this->path_daily               = "";
      $this->path_weekly              = "";
      $this->path_monthly             = "";
      $this->path_backupset           = "";
      $this->path_backupset_folder    = "";
      $this->current_subfolder        = "";

      $this->backupset                = new backupvfs_backupset_c( $application_object, $this );
      $this->backupset_name           = "";

      $this->usehardlinks             = true;
      $this->keeplocal                = true;

      $this->umaskfolders             = 511; // octal 777
      $this->umaskfiles               = 438; // octal 666

      $this->minfreespace             = 0;
      $this->minbackupsize            = 0;

      $this->rotation_enabled         =  false;// flag true if rotations are enabled for this vfs
      $this->rotation_dayofweek       =  0;    // day of the week to rotate daily backups
      $this->rotation_dayofmonth      =  1;    // day of the month to rotate monthly backups
      $this->rotation_total_daily     =  7;    // total number of daily backups in the daily rotation
      $this->rotation_total_weekly    = 12;    // total number of weekly backups in the weekly rotation
      $this->rotation_total_monthly   = 36;    // total number of monthly backups in the monthly rotation

      $this->named_parameters               = new named_parameters_c( "sysbackup", "vfs-base" );

      $np_method = $this->np_default_method = "vfs-base";
      $np_index  = $this->np_default_index  = "vfs-base";

      $this->named_parameters->register( $np_method, $np_index, array( 
          "enabled"                => array( 1, NPREQ, "plugin_enabled", "true if the plugin is enabled" ),

          "hostname"               => array( 3, false, false, "Name of the target host for this object" ),
          "hostid"                 => array( 3, false, false, "Short version of the host name" ),

          "keeplocal"              => array( 3, false, false, "Flag true if saving a full backup on this filesystem" ),
          "usehardlinks"           => array( 1, NPREQ, "usehardlinks",   "true if remote server supports hardlinks" ),

          "path_temp"              => array( 3, false, false, "Path to the temp work space"                                 ),
          "path_daily"             => array( 3, false, false, "Path to the backupsets on this filesystem"                   ),
          "path_weekly"            => array( 3, false, false, "Path to the weekly backupset rotations on this filesystem"   ),
          "path_monthly"           => array( 3, false, false, "Path to the monthly backupset rotations on this filesystem"  ),

          "rotation_enabled"       => array( 3, false, false, "true if rotations are enabled on the remote system"          ),
          "rotation_dayofweek"     => array( 3, false, false, "day of week on which the weekly rotation is generated"       ),
          "rotation_dayofmonth"    => array( 3, false, false, "day of month on which the monthly rotation is generated"     ),
          "rotation_total_daily"   => array( 3, false, false, "total number of backupsets that are in the daily rotaiton"   ),
          "rotation_total_weekly"  => array( 3, false, false, "total number of backupsets that are in the weekly rotaiton"  ),
          "rotation_total_monthly" => array( 3, false, false, "total number of backupsets that are in the monthly rotaiton" ),

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
      $this->app->tee( $this->plugin_title );
      $this->named_parameters->dump_properties( $this, $this->np_default_method, $this->np_default_index );
      $this->app->tee("");
   }


   function stat_str( $type )
   {
      static $myvars = array( "start", 
                              "tarball_count",    "tarball_size", 
                              "hardlinked_count", "hardlinked_size" );

      $stats = $this->get_stats();
      foreach ( $myvars as $v )
      {
         $n = "{$type}_{$v}";
         $$v = $stats[$n];
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
   function _std_plugin_method_warning( $op )
   {
      $this->app->warning( "from plugin {$this->plugin_name}: {$op}" );
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


   function vfs_init( $backupset_name )
   {
      $this->backupset_name = $backupset_name;

      if ( $this->vfs_warn_disk_space( true ) )
         $this->fatal( "problems found - please correct and try again" );

      if ( $this->app->appmode_exec() )
         return $this->app->vfs_createbackupset_folder( $this->path_daily );

      return false;
   }


   function vfs_start()
   {
      $this->_mark_backupset_incomplete();

      if ( $this->backupset->get_last_hashlist( $this->path_daily ) )
      {
         $current_backupset  = $this->get_path_backupset();
         $prev_backupset     = $this->backupset->get_prev_path();
         $r                  = $this->_vfs_can_hardlink( $prev_backupset, $current_backupset );

         if ( $this->usehardlinks != $r )
         {
            $this->warning( "Setting UseHardLinks to " . phpval2str( $r )
                          . " (was set to " . phpval2str( $this->usehardlinks ) . ")" );

            $this->usehardlinks = $r;
         }
      }
   }


   function vfs_end()
   {
      $this->backupset->current_hashlist->save_all();

      if ( $this->app->verbose() )
         $this->tee( "writing last/prev backup names on " . $this->hostid() );

      $this->backupset->save_last_backup_name( $this->path_daily );

      $this->_unmark_backupset_incomplete();

      // always returns no error result
      return false; 
   }


   function vfs_addfile( $target, $source )
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
}

