<?php // -*- mode: php; mode: mmm; coding: utf-8-unix; -*-
/** 
 *
 * Virtual Backup Filesystem plugin
 *
 * <p>The backup_vfs_base_c class defines a basic set of file handling
 * methods for use by the various plugins. The idea is that all
 * plugins will use the same set of simple methods to move files and
 * tarballs of files into the backup set. Those methods will then deal
 * with the issue of copying the files into the local, remote or both
 * file stores. The plugins therefore don't need to know anything
 * about the backup set - they only need to know about the job they
 * are trying to perform.
 *
 * <p><b>Procedure</b>
 *
 * <p>The basic procedure will be as described below. The backup
 * system will instantiate a master class that will keep track of all
 * target filesystems to be used see the notes below about VM's that
 * backup to dom0 as well as a separate NAS device.) Each different
 * filesystem will be managed by a class that will be able to do the
 * following:
 *
 * <ul> 
 *
 * <li>The constructor will:
 *
 * <p>- Determine the location of the temporary workspace<br>
 * <br>
 * -Determine the name and location of the backup (the target
 *  folder for the backup being produced by the current program
 *    run.)<br>
 *<br>
 * - Find the name and location of the previous successful
 *    backup, if any.<br>
 *<br>
 * - Get the hashlist for the files that were written to the last
 *    successful backup (allowing the use of hardlinks to existing
 *    files in cases where those files are identical.)<br>
 *<br>
 *
 * <li>The class will provide a method for adding a sub-folder within
 * the backup set and selecting that sub-folder for use during a given
 * backup job.</li>
 *
 * <li>The class will provide a method for compressing output piped
 * from a process and adding it to the backup set.</li>
 *
 * <li>The class will provide a method for compressing a file on the
 * local filesystem and adding it to the backup set.</li>
 *
 * <li>The class will provide a method for tarballing a folder on the
 * local filesystem and adding it to the backup set.</li>
 *
 * <li>The class will list separately the tarballs that are hardlinked
 * so that future code can check for bit-rot.</li>
 *
 * <li>The class will keep track of available disk space on the target
 * filesystem and on the local filesystem. The class will also keep
 * track of all items added to the backup set. If disk space on the
 * local filesystem is limited the class, when writing to a remote
 * filesystem, the class will copy the file immediately to allow the
 * local copy to be deleted. If a full backup is being saved to the
 * local filesystem the class will queue files for remote copy and
 * perform the copies after all other backup tasks have been
 * completed.</li>
 *
 * <li>The class will implement a set of counters that will keep track
 * of disk space used, number of files written, number of files
 * hardlinked and any related information.</li>
 *
 * <li>The class will provide a number of methods for use by the
 * backup rotation plugin (create folders, list folders and recursive
 * delete of folders.)</li>
 *
 * </ul>
 *
 * <p>Note that this backup software will often be running on a Virtual
 * Machine with limited available disk storage. In many cases the VM
 * will need to backup directly to the dom0 while using the minimum
 * possible disk space on the VM. However, the dom0 is not suitable
 * for use as remote backup storage. In most cases the backup script
 * will also be forced to send the same backup set to a NAS device on
 * the network. Finally, after creating the full backup set on the
 * dom0 and the NAS device, this script will also need to trigger a
 * synchronization process between the local NAS device and an offsite
 * storage server. 
 *
 * <p>As such the backup VFS must be able to operate on the local
 * filesystem as well as two different remote filesystems.
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



require_once "lib/sys-tarballs.inc";
require_once "lib/sys-remote-fileio.inc";

require_once "app/sysbackup-vfs-io.inc";
require_once "app/sysbackup-vfs-hashlists.inc";
require_once "app/sysbackup-vfs-backupset.inc";
require_once "app/sysbackup-vfs-base.inc";
require_once "app/sysbackup-vfs-localio.inc";
require_once "app/sysbackup-vfs-localio-rotation.inc";
require_once "app/sysbackup-vfs-sshremote.inc";
require_once "app/sysbackup-vfs-sshremote-rotation.inc";




/**
 *
 * @package sysbackup
 * @subpackage sysbackup
 *
 */
class backupvfs_main_c extends plugin_framework_c
{
   private $sys_timezone;        // default timezone to keep PHP happy

   private $backupset_namefmt;   // format string for the date() function to generate backupset names
                                 // embed \%\H\O\S\T\% to output %HOST% which is replaced with $sys_hostname
   private $backupset_name;      // name of the current backupset

   private $tar;                 // The object used to compress files and folders into tarballs

   private $sys_local;           // The local VFS object
   private $sys_local_weekly;    // the weekly rotation object
   private $sys_local_monthly;   // the monthly rotation object

   private $sys_remote;          // An array of remote VFS objects (id => $obj, id => obj, ... )
   private $sys_remotes;         // An array of remote VFS object names (id, id, ... )

   private $current_subfolder;   // a string containing the name of the most recently selected sub-folder 
                                 // in the backupset.

   private $max_job_subfolder_width; // max width of a subfolder name to help improve the appearance
                                 // of the "not-quiet" display output

   // array of files queued for transfer
   private $xfer_queue;          // queue[] = array( $rel_ref, $sha1, $size ), ...



   function set_adminemail( $e )       { $this->admin_email  = $e;              }
   function set_timezone( $t )         { $this->sys_timezone = $t;              }
   function set_dir_config( $d )       { $this->app_confdir  = $d;              }
   function set_dir_plugins( $d )      { $this->app_plugindir = $d;             }

   function get_adminemail()           { return $this->admin_email;             }
   function get_timezone()             { return $this->sys_timezone;            }
   function get_dir_plugins()          { return $this->app_confdir;             }
   function get_dir_config()           { return $this->app_confdir;             }

   function set_hostname( $nam, $id )  { $this->sys_local->hostname = $nam; 
                                         $this->sys_local->hostid   = $id;      }

   function get_hostname()             { return $this->sys_local->hostname;     }
   function get_hostid()               { return $this->sys_local->hostid;       } 

   function set_nameformat( $fmt )     { $this->backupset_namefmt = $fmt;       }
   function get_backupset_name()       { return $this->backupset_name;          }

   function set_path_temp( $dir )      { $this->sys_local->path_temp    = $dir; }
   function set_path_daily( $dir )     { $this->sys_local->path_daily   = $dir; }
   function set_path_weekly( $dir )    { $this->sys_local->path_weekly  = $dir; }
   function set_path_monthly( $dir )   { $this->sys_local->path_monthly = $dir; }

   function get_path_temp()            { return $this->sys_local->path_temp;    }
   function get_path_daily()           { return $this->sys_local->path_daily;   }
   function get_path_weekly()          { return $this->sys_local->path_weekly;  }
   function get_path_monthly()         { return $this->sys_local->path_monthly; }

   function get_path_backupset()       { return $this->sys_local->path_backupset;    }
   function get_current_subfolder()    { return $this->sys_local->current_subfolder; }


  /**
   * returns an array of [id:which] => path<temp, daily, weekly & monthly> for each VFS
   *
   * ie: if the local filesystem is on dev3.int.azertech.net and the path_temp is /tmp/
   *     the return array will include "[dev3:temp]" => "/tmp/"
   *
   * This information is used (or will be used - a code review is
   * needed) will be used to simplify and improve the readability of
   * error messages
   *
   */
   function get_paths( $id = false ) 
   { 
      if ( $id === false )
      {
         $p = $this->sys_local->get_paths( $this->get_hostid() );
         foreach ( $this->sys_remotes as $id )
            $p = array_merge( $p, $this->remote[$id]->get_paths() );
      }

      if ( $id == $this->get_hostid() )
         return $this->sys_local->get_paths();

      foreach ( $this->sys_remotes as $i )
         if ( $i == $id )
            return array_merge( $p, $this->remote[$id]->get_paths() );

      return false;
   }


   function set_keeplocal( $f )        { $this->sys_local->keeplocal = $f;      }
   function get_keeplocal()            { return $this->sys_local->keeplocal;    }

   function set_usehardlinks( $f )     { $this->sys_local->usehardlinks = $f;   }
   function get_usehardlinks()         { return $this->sys_local->usehardlinks; }

   function set_minfreespace( $b )     { $this->sys_local->minfreespace = $b;   }
   function get_minfreespace()         { return $this->sys_local->minfreespace; }

   function set_minbackupsize( $b )    { $this->sys_local->minbackupsize = $b;  }
   function get_minbackupsize()        { return $this->sys_local->minbackupsize;}

   function set_umasks( $dir, $fil )   { $this->sys_local->umaskfolders = $dir;
                                         $this->sys_local->umaskfiles   = $fil; }

   function get_umaskfolders()         { return $this->sys_local->umaskfolders; }
   function get_umaskfiles()           { return $this->sys_local->umaskfiles;   }

   function set_rotation_enabled( $f ) { $this->sys_local->rotation_enabled       = $f;}
   function set_rotation_dayofweek($f) { $this->sys_local->rotation_dayofweek     = $f;}
   function set_rotation_dayofmonth($f){ $this->sys_local->rotation_dayofmonth    = $f;}
   function set_rotations_daily( $f )  { $this->sys_local->rotation_total_daily   = $f;}
   function set_rotations_weekly( $f ) { $this->sys_local->rotation_total_weekly  = $f;}
   function set_rotations_monthly( $f ){ $this->sys_local->rotation_total_monthly = $f;}

   function get_rotation_enabled()     { return $this->sys_local->rotation_enabled;    }
   function get_rotation_dayofweek()   { return $this->sys_local->rotation_dayofweek;  }
   function get_rotation_dayofmonth()  { return $this->sys_local->rotation_dayofmonth; }
   function get_rotations_daily()      { return $this->sys_local->rotation_total_daily;}
   function get_rotations_weekly()     { return $this->sys_local->rotation_total_weekly;}
   function get_rotations_monthly()    { return $this->sys_local->rotation_total_monthly;}


   // return the list of remote storage ID's.
   function get_remotes()              { return $this->remotes;                        }


   function &get_tar_object()
   {
      return $this->tar;
   }


   function set_debugsysexec( $flg = -1 )
   {
      if ( $flg === -1 )
         return $this->debug_sysexec;

      $flg = parent::set_debugsysexec( $flg );

      $this->tar->set_debugsysexec($flg);
      $this->sys_local->set_debugsysexec($flg);

      foreach( $this->sys_remotes as $id )
         $this->sys_remote[$id]->set_debugsysexec($flg);

      return $flg;
   }



   function __construct( $opts = false )
   {
      parent::__construct( $opts );

      $np_method = "sysbackup-app";
      $np_index  = "base";

      // TODO: get the named parameters installed here

      $this->tar                = new mktarball_util_c();

      $this->sys_local          = new backupvfs_localio_c( $this, $this->tar, $opts );
      $this->sys_local_weekly   = new backupvfs_localio_rotation_plugin_c( $this, $this->tar, "weekly",  $opts );
      $this->sys_local_monthly  = new backupvfs_localio_rotation_plugin_c( $this, $this->tar, "monthly", $opts );

      $this->sys_remote         = array();
      $this->sys_remotes        = array();

      $this->sys_remote_weekly  = array();
      $this->sys_remote_monthly = array();
   }


  /*****************************************************************************
   *
   * Internal Plugin functionality
   *
   *****************************************************************************/

   private function _remote_xferfile( $target, $hash, $size )
   {
      foreach( $this->sys_remotes as $id )
      {
         $start = microtime(true);
         if ( $this->notquiet() )
            $this->tee( "  {$id}:", "" );

         $r   = $this->sys_remote[$id]->remote_copyfile( $target, $hash, $size );

         if ( $this->sys_remote_weekly[$id]->plugin_enabled )
            $ref = $this->sys_remote_weekly[$id]->rotation_copyfile( $target, $hash, $size );

         if ( $this->sys_remote_monthly[$id]->plugin_enabled )
            $ref = $this->sys_remote_monthly[$id]->rotation_copyfile( $target, $hash, $size );

         $tim = microtime(true) - $start;
         $dur = $this->duration($tim);
         $hl  = $this->sys_remote[$id]->is_hardlinked( $target );

         if ( $this->notquiet() )
            $this->tee( ($hl ? "*" : "") . "{$dur}s" . ($r ? "??" : ""), "" );
      }
   }


   function remote_xferfiles()
   {
      if ( $this->not_quiet() )
         $this->summary_stats();

      $this->vfs_subfolder("");

      if ( !$this->get_keeplocal() )
         return false;
         
      if ( !count($this->sys_remotes) )
         return false;

      foreach( $this->sys_remotes as $id )
         $this->sys_remote[$id]->all_reset();

      $fname_width = 0;
      foreach( $this->xfer_queue as $finfo )
         if ( ($w = strlen($finfo[0])) > $fname_width )
            $fname_width = $w;

      foreach( $this->xfer_queue as $finfo )
      {
         $this->_notquiet_report_header();

         $fn   = str_pad( $finfo[0], $fname_width );
         $size = str_pad( si_suffix($finfo[2], "B"), 8, " ", STR_PAD_LEFT );
         $this->tee( "    {$size} {$fn}", "" );

         $this->_remote_xferfile( $finfo[0], $finfo[1], $finfo[2] );

         if ( $this->notquiet() )
            $this->tee( "", "\n" );
      }

      return false;
   }



   function prune_expired_backups()
   {
      $this->sys_local->vfs_prune_expired_backups();

      if ( $this->sys_local_weekly->plugin_enabled )
         $this->sys_local_weekly->vfs_prune_expired_backups();

      if ( $this->sys_local_monthly->plugin_enabled )
         $this->sys_local_monthly->vfs_prune_expired_backups();

      foreach( $this->sys_remotes as $id )
      {
         $ref = $this->sys_remote[$id]->vfs_prune_expired_backups();

         if ( $this->sys_remote_weekly[$id]->plugin_enabled )
            $ref = $this->sys_remote_weekly[$id]->vfs_prune_expired_backups();

         if ( $this->sys_remote_monthly[$id]->plugin_enabled )
            $ref = $this->sys_remote_monthly[$id]->vfs_prune_expired_backups();
      }
   }



   /**
    *
    * Internal Plugin
    *
    * Job information that references the Internal Plugin are sent to
    * this function.
    *
    * Note that the name of the internal plugin is assigned to the
    * PLUGGABLEAPP_PLUGIN_INTERNAL define. It is currently 'internal'
    *
    * Internal jobs all have a method name. Some of the methods
    * require additional parameters.
    *
    */

   function plugin_internal( $job )
   {
      switch( $job['method'] )
      {
         case 'remote_xferfiles':
            $this->remote_xferfiles();
            break;

         case 'prune_expired_backups':
            $this->prune_expired_backups();
            break;

         case 'exec':
            if ( !isset( $job['command'] ) )
               $this->fatal( "the internal exec method requires a command" );

            $cmd = $job['command'];

            if ( $this->verbose() )
               $this->tee( "internal(exec '{$cmd}') returns:" );

            $r = `{$cmd}`;

            if ( $this->verbose() )
               $this->tee( $r );

            break;


         default:
            $this->fatal( "uknown internal method name: {$job['method']}" );
            break;
      }

      return;
   }


   function add_standard_internal_jobs()
   {
      $this->add_job( "[internal]", 
          array( "id"          => "xfer-queued",
                 "description" => "Transfer the new backupset to remote storage",
                 "enabled"     => true,

                 "event"       => "auxops",
                 "jobindex"    => "5000.RemoteXfer",

                 "plugin"      => PLUGGABLEAPP_PLUGIN_INTERNAL,
                 "method"      => "remote_xferfiles",
                ));

      $this->add_job( "[internal]", 
          array( "id"          => "prune-expired",
                 "description" => "Prune Expired Backups from the Rotation folders",
                 "enabled"     => true,

                 "event"       => "auxops",
                 "jobindex"    => "7500.PruneExpired",

                 "plugin"      => PLUGGABLEAPP_PLUGIN_INTERNAL,
                 "method"      => "prune_expired_backups",
                ));
   }


  /*****************************************************************************
   *
   * End of Internal Plugin code
   *
   *****************************************************************************/


   function setup_compression( $opts )
   {
      $this->tar->setup( $opts );
   }


   function warn_disk_space( $full = false )
   {
      $this->sys_local->vfs_warn_disk_space( $full );
   }


   function add_sshremote( $id, $opts )
   {
      if ( isset($opts["enabled"]) && $opts["enabled"] )
      {
         $this->sys_remotes[]           = $id;
         $this->sys_remote[$id]         = new backupvfs_sshremote_c( $this, $opts );
         $this->sys_remote_weekly[$id]  = new backupvfs_sshremote_rotation_c( $this, $this->sys_remote[$id], "weekly", $opts );
         $this->sys_remote_monthly[$id] = new backupvfs_sshremote_rotation_c( $this, $this->sys_remote[$id], "monthly", $opts );
      } 
        else $this->tee( "Note: {$id}@{$opts['remote_server']} is disabled" );
   }


   function dump_config()
   {
      parent::dump_config();

      $this->sys_local->dump_config();

      foreach( $this->sys_remotes as $id )
         $this->sys_remote[$id]->dump_config();

      $this->tar->dump_config( $this );
      $this->tee("");
   }


   function vfs_init( $backupset_name  = false )
   {
      if ( $backupset_name === false )
      {
         $backupset_name = str_replace( "%HOST%", 
                                        $this->get_hostname(),
                                        date($this->backupset_namefmt) );

         $backupset_name = str_replace( "%HOSTID%", 
                                        $this->get_hostid(),
                                        $backupset_name );
      }

      $this->backupset_name = $backupset_name;

      $this->sys_local->vfs_init($backupset_name);
      $this->sys_local_weekly->vfs_init($backupset_name);
      $this->sys_local_monthly->vfs_init($backupset_name);
   }


   function log_stats( $type )
   {
      $tot = 0;
      $counter = "{$type}_tarball_count";
      $stats = array();
      foreach( $this->sys_remotes as $id )
         $stats[$id] = $this->sys_remote[$id]->get_stats();

      foreach( $this->sys_remotes as $id )
         $tot += $stats[$id][$counter];

      $this->log( "{$type}: " 
                . $this->sys_local->plugin_shortref()
                . " " 
                . $this->sys_local->stat_str( $type )
                );

      if ( !$tot )
         return false;

      foreach( $this->sys_remotes as $id )
         $this->log( "{$type}: "
                   . $this->sys_remote[$id]->plugin_shortref()
                   . " " 
                   . $this->sys_remote[$id]->stat_str( $type )
                   );

      return false;
   }


   function show_stats( $type )
   {
      if ( $this->quiet() )
         return false;

      $width = 0;
      $labels = array();

      $index = 0;
      if ( ($w = strlen($labels[$index++] = $this->sys_local->plugin_shortref())) > $width ) 
         $width = $w;

      foreach( $this->sys_remotes as $id )
         if ( ($w = strlen($labels[$index++] = $this->sys_remote[$id]->plugin_shortref())) > $width )
            $width = $w;

      $index = 0;
      $this->tee( "  -"
                . str_pad( $labels[$index++], $width, "-", STR_PAD_LEFT ) 
                . ": " 
                . $this->sys_local->stat_str( $type ) );

      $stats = array();
      $tot = 0;
      $counter = "{$type}_tarball_count";
      foreach( $this->sys_remotes as $id )
         $stats[$id] = $this->sys_remote[$id]->get_stats();

      foreach( $this->sys_remotes as $id )
         $tot += $stats[$id][$counter];

      if ( !$tot )
         return false;

      foreach( $this->sys_remotes as $id )
         $this->tee( "  -"
                   . str_pad( $labels[$index++], $width, "-", STR_PAD_LEFT ) 
                   . ": " 
                   . $this->sys_remote[$id]->stat_str( $type ) );

      return false;
   }


   function summary_stats()
   {
      if ( $this->quiet() )
         return false;

      $this->tee( "" );
      $this->tee( "Summary Stats" );
      $this->show_stats( "current" );
      $this->tee( "" );

      $this->log_stats( "current" );

      return false;
   }


   function job_reset()
   {
      $this->sys_local->job_reset();
      foreach( $this->sys_remotes as $id )
         $this->sys_remote[$id]->job_reset();
   }


   function step_reset()
   {
      $this->sys_local->step_reset();
      foreach( $this->sys_remotes as $id )
         $this->sys_remote[$id]->step_reset();
   }


   //TODO: great idea but it can't work - remove this!!
   function vfs_displayref( $ref )
   {
      $ref = $this->sys_local->vfs_displayref( $ref );

      if ( $this->sys_local_weekly->plugin_enabled )
         $ref = $this->sys_local_weekly->vfs_displayref( $ref );

      if ( $this->sys_local_monthly->plugin_enabled )
         $ref = $this->sys_local_monthly->vfs_displayref( $ref );

      foreach( $this->sys_remotes as $id )
      {
         $ref = $this->sys_remote[$id]->vfs_displayref( $ref );

         if ( $this->sys_remote_weekly[$id]->plugin_enabled )
            $ref = $this->sys_remote_weekly[$id]->vfs_displayref( $ref );

         if ( $this->sys_remote_monthly[$id]->plugin_enabled )
            $ref = $this->sys_remote_monthly[$id]->vfs_displayref( $ref );
      }

      return $ref;
   }


   function vfs_start()
   {
      $this->sys_local->vfs_start();

      if ( $this->sys_local_weekly->plugin_enabled )
         $this->sys_local_weekly->vfs_start();

      if ( $this->sys_local_monthly->plugin_enabled )
         $this->sys_local_monthly->vfs_start();

      foreach( $this->sys_remotes as $id )
      {
         $this->sys_remote[$id]->vfs_init( $this->get_backupset_name() );
         $this->sys_remote_weekly[$id]->vfs_init( $this->get_backupset_name() );
         $this->sys_remote_monthly[$id]->vfs_init( $this->get_backupset_name() );
      }

      foreach( $this->sys_remotes as $id )
      {
         $this->sys_remote[$id]->vfs_start();

         if ( $this->sys_remote_weekly[$id]->plugin_enabled )
            $this->sys_remote_weekly[$id]->vfs_start();

         if ( $this->sys_remote_monthly[$id]->plugin_enabled )
            $this->sys_remote_monthly[$id]->vfs_start();
      }
   }


   function vfs_end()
   {
      $this->sys_local->vfs_end();

      if ( $this->sys_local_weekly->plugin_enabled )
         $this->sys_local_weekly->vfs_end();

      if ( $this->sys_local_monthly->plugin_enabled )
         $this->sys_local_monthly->vfs_end();

      foreach( $this->sys_remotes as $id )
      {
         $this->sys_remote[$id]->vfs_end();

         if ( $this->sys_remote_weekly[$id]->plugin_enabled )
            $this->sys_remote_weekly[$id]->vfs_end();

         if ( $this->sys_remote_monthly[$id]->plugin_enabled )
            $this->sys_remote_monthly[$id]->vfs_end();
      }
   }


   function vfs_subfolder( $folder_name )
   {
      $this->current_subfolder = $folder_name;

      $this->sys_local->vfs_subfolder( $folder_name );

      if ( $this->sys_local_weekly->plugin_enabled )
         $this->sys_local_weekly->vfs_subfolder( $folder_name );

      if ( $this->sys_local_monthly->plugin_enabled )
         $this->sys_local_monthly->vfs_subfolder( $folder_name );

      foreach( $this->sys_remotes as $id )
      {
         $this->sys_remote[$id]->vfs_subfolder( $folder_name );

         if ( $this->sys_remote_weekly[$id]->plugin_enabled )
            $this->sys_remote_weekly[$id]->vfs_subfolder( $folder_name );

         if ( $this->sys_remote_monthly[$id]->plugin_enabled )
            $this->sys_remote_monthly[$id]->vfs_subfolder( $folder_name );
      }
   }


   function _vfs_local_cleanup( $rel_ref )
   {
      // are we keeping this file?
      if ( $this->sys_local->get_keeplocal() )
         return false; // yes, return okay

      // no, delete it.
      $abs_ref = dir_fn( $this->get_path_backupset(), $rel_ref );

      // sanity check
      if ( !file_exists( $abs_ref ) ) // unexpected condition (programming error?)
         $this->fatal( "expired file not found {$abs_ref}" );

      if ( unlink( $abs_ref ) )
         $this->fatal( "unable to remove expired file {$abs_ref}" );

      return false;
   }


   function _verbose_report( $target, $type = "file" )
   {
      $sub = $this->get_current_subfolder() 
                ? $this->get_current_subfolder() 
                : "/";

      if ( $this->verbose() )
         $this->tee( "    {$sub} {$type} {$target}" );
   }


   function _notquiet_report_header( $reset = false )
   {
      static $last_event;
      static $last_jobindex;

      if ( $reset )
      {
         $last_event = -1;
         $last_jobindex = -1;
         return;
      }

      if ( $this->quiet() )
         return false;

      $event_changed = false;
      if ( $last_event != $this->current_event )
      {
         if ( $last_event !== -1 )
            $this->tee( "" );

         $last_event = $this->current_event;
         $this->tee( "{$last_event}: {$this->event_descriptions[$last_event]}" );
         $event_changed = true;
      }

      if ( $last_jobindex != $this->current_jobindex )
      {
         $last_jobindex = $this->current_jobindex;

         if ( !$event_changed ) 
            $this->tee( "" );

         $this->tee( "  {$last_jobindex}: {$this->current_job['plugin']}({$this->current_job['id']})" );
      }
   }


   function _notquiet_report( $target, $eol = "\n" )
   {
      if ( $this->quiet() )
         return false;

      $this->_notquiet_report_header();

      $sub        = str_pad( $this->current_subfolder 
                             ? $this->current_subfolder 
                             : "/", 
                             $this->max_job_subfolder_width );

      $size       = str_pad( si_suffix($this->sys_local->filesize( $target ), "B"), 
                                10, " ", STR_PAD_LEFT );

      $hardlinked = $this->sys_local->is_hardlinked( $target ) ? "*" : "";

      $this->tee( "    {$sub} {$size} {$hardlinked}{$target}", $eol );
   }


   private function vfs_addremote( $rel_ref )
   {
      $hash = $this->sys_local->filehash( $rel_ref );
      $size = $this->sys_local->filesize( $rel_ref );

      $this->_notquiet_report( $rel_ref, "" );

      if ( $this->sys_local_weekly->plugin_enabled )
         $this->sys_local_weekly->rotation_copyfile( $rel_ref, $hash, $size );

      if ( $this->sys_local_monthly->plugin_enabled )
         $this->sys_local_monthly->rotation_copyfile( $rel_ref, $hash, $size );

      if ( $this->sys_local->get_keeplocal() )
      {
         if ( count($this->sys_remotes) )
            $this->xfer_queue[] = array( $rel_ref, $hash, $size );

         if ( $this->notquiet() )
            $this->tee( "", "\n" );

         return false;
      }

      $this->_remote_xferfile( $rel_ref, $hash, $size );

      if ( $this->notquiet() )
         $this->tee( "", "\n" );

      return $this->_vfs_local_cleanup( $rel_ref );
   }


   function vfs_addfile( $target, $source )
   {
      $this->_verbose_report( $target );

      if ( $this->sys_local->vfs_addfile( $target, $source ) )
         $this->fatal( "unable to add {$target} to the local backupset" );

      return $this->vfs_addremote( $target );
   }


   function vfs_compress_string( $target, $source_str )
   {
      $this->_verbose_report( $target );

      if ( $this->sys_local->vfs_compress_string( $target, $source_str ) )
         $this->fatal( "unable to add {$target} to the local backupset" );

      return $this->vfs_addremote( $target );
   }


   function vfs_compress_command( $cmd, $target )
   {
      $this->_verbose_report( $target );

      if ( $this->sys_local->vfs_compress_command( $cmd, $target ) )
         $this->fatal( "unable to add {$target} to the local backupset" );

      return $this->vfs_addremote( $target );
   }


   function vfs_compress_file( $target, $source )
   {
      $this->_verbose_report( $target );

      if ( $this->sys_local->vfs_compress_file( $target, $source ) )
         $this->fatal( "unable to add {$target} to the local backupset" );

      return $this->vfs_addremote( $target );
   }


   function vfs_tarball_folder( $target, $source, $options = array() )
   {
      $this->_verbose_report( $target, "folder" );

      if ( $this->sys_local->vfs_tarball_folder( $target, $source, $options ) )
         $this->fatal( "unable to add {$target} to the local backupset" );

      return $this->vfs_addremote( $target );
   }
}



