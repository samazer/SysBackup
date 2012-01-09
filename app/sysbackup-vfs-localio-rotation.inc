<?php /* -*- mode: php; mode: mmm; coding: utf-8-unix; -*- */
/**
 *
 * Local IO Rotation Plugin
 *
 * A plugin to automatically rotate backupsets so that a certain
 * number are online at all times (without going over that number.)
 * This is intended to help automatically manage disk space.
 * Configuration information inherited by this class is not actually
 * used - this class refers to the config info in the associated
 * localio VFS object.
 *
 * Note that these rotation plugins are not actually loaded as plugins
 * or as virtual filesystems. These are objects that interact with the
 * corresponding VFS objects. They are implemented as separate VFS
 * plugins so that they can load hash tables for the rotation folders
 * using the same code that is used by the VFS plugins.
 *
 * <p><b>What's a Rotation anyway?</b>
 *
 * Many years ago backups were written to tapes - large reels of
 * tape. Tape was expensive and difficult to work with. It would
 * become unreadable if it got crinkled and there were many little
 * problems that would cause the tape to go flying off the reel and
 * end-up all over the floor in a big mess. In my personal case I only
 * used reels of tape for music in the 70's. By the 80's tape had
 * fallen out of favor among audiophiles and tape cassettes had become
 * the more common format. Data, however, was still being recorded on
 * large tape reels.
 *
 * Personally I did not start using tape for data backup until some
 * time in the 80's when data cartridges became popular - so I never
 * really had to suffer the horror of big reels of data tape all over
 * the office floor. However I did have my share of problems with data
 * cartridges. They were a huge improvement over reels of tape. They
 * were easier to use and the tape rarely came out of the cartridge
 * where it belonged. Still, cartridges were expensive and, again, the
 * tape would easily become unreadable after being used a few times.
 *
 * So, we needed to be able to record our nightly backups onto the
 * cartridges without having the tape get crinkled and become
 * unreadable. Unfortunately the cartridges were painfully
 * expensive. We didn't have enough money to go out and buy one
 * cartridge for each day of the year. The most effective course of
 * action was to buy as many cartridges as we could afford and use
 * them sequentially. When we got to the end of a stack of cartridges
 * we would go back to the first one and start again. This was called
 * a ROTATION and it allowed all the previously used tapes in the
 * stack to be available in case of a problem. 
 *
 * In practice the tape would get crinkled and the cartridges would be
 * found unreadable after a few uses. If the previously recorded
 * backup had become unreadable due to crinkles in the tape... we just
 * tried the one before that, and so on, until we found one that was
 * readable. Sometimes we would manage to recover some files from one
 * cartridge and would look for more files on other cartridges. This
 * was not fun.
 *
 * We also used additional weekly and monthly rotations. This was in
 * part to extend the duration of the backup so that backupsets
 * covered a longer period of time. However, it was also a way of
 * ensuring that the cartridges would last longer before becoming
 * unreadable.
 *
 * The rotation process for the weekly and monthly backups was as
 * simple as the daily rotation process: Every Sunday the tape from
 * the previous Sunday would be rotated into a weekly backup
 * stack. Every month we might (if we were able to purchase enough
 * cartridges,) also rotate the weekly backup from the start of the previous
 * month into a monthly backup stack. Each week the expired weekly and
 * monthly cartridges would then go back into the daily rotation. This
 * ensured that there were always tapes in the daily rotation that
 * were less heavily used and, therefore, more likely to be reliable.
 *
 * As you can imagine this was a frustrating exercise.  Consequently
 * tape cartridges were never really very popular. Over time the cost
 * of hard disk drives had gone down to the point where some companies
 * starting using hard disk storage instead of tape storage in their
 * backup rotations. I had one customer in the 90s who had a computer
 * dedicated to the task of making backups. The drive bays on that
 * computer were all removable. The president of the company would
 * take one weekly backup home on Friday night and, on Monday morning,
 * would return with the previous weekly backup drive and put it back
 * into the company hard disk rotation. This ensured that the company
 * would have an off-site backup available (in his home) in the event
 * of a disaster at the office.
 *
 * In the current age of LVM snapshots, inexpensive hard disk storage,
 * Network Addressable storage and Cloud-based storage there are a
 * wide range of backup options available. It is now possible to take
 * a snapshot of a busy filesystem, back it up to a long-term local
 * rotation and copy it simultaneously to remote storage - possibly
 * even off-site.
 *
 * The availability of a long-term rotation on a local server improves
 * dramatically the response to typical user issues (especially if the
 * backup is fine-grained.) A user who has accidentally deleted a
 * file, for example, might not realize for several days that he has a
 * problem. When he does finally call the I.T. department for help the
 * daily rotation may have expired - but a copy of the file might be
 * found on the weekly and/or monthly rotations. By keeping the
 * backups as fine-grained as possible it is easy to find an
 * appropriate tarball and make it available to a user whenever
 * necessary.
 *
 * <p><b>How it works</b>
 *
 * <p>Files are added to this Virtual File System as they are for the
 * localio Virtual File System. However, the LocalIO VFS must be
 * allowed to work as quickly as possible to ensure that filesystem
 * snapshots are released quickly. Therefore the rotations plugins are
 * designed to queue any files that need to be copied and copy them
 * after the main backupset is created.
 *
 * <p>Specifically:
 *
 * <ul>
 *
 * <li>On load this plugin determines if it must write weekly or
 * monthly backups and sets appropriate flags.
 *
 * <li>On startup this plugin follows the same procedure that was
 * followed by the localio plugin except that it operates on the
 * rotation folder instead of the backup folder. In other words it
 * finds the previously written backupset in the Rotation folder (if
 * any) and loads the associated hashlist.
 *
 * <li>If weekly and/or monthly rotations are being written: All files
 * written to the main backupset are copied to the weekly and/or
 * monthly rotations. Hardlinks are always used if the same partitions
 * are used by the backup and rotations folders. If the partitions are
 * different the current hashlist is used to determine if an exact
 * copy of a file already exists in the previously written backupset
 * on the rotations folder. If so a hardlink is used; if not the file
 * is queued for later copying.
 *
 * <li>When all other tasks are completed the following tasks will be
 * executed:
 *
 * <p>(1) All queued file copy operations will be completed.
 *
 * <p>(2) The daily, weekly and monthly folders will be pruned. See
 * the notes below. Pruning is not done before the backup in case the
 * process fails. This ensures that there is at least one good backup
 * available on the local filesystem (the previous one, if any.)
 * Unfortunately this requires that local filesystems, if they will be
 * keeping any local copies of backups at all, require enough disk
 * space available at all times for at least two complete backups. (If
 * this is a problem on some VM's the last backup can always be
 * removed before creating the next backup.)
 *
 * <p><b>Pruning Folders</b>
 *
 * <p>This plugin takes a simple approach to managing rotations:
 *
 * <ul>
 *
 * <li>Daily rotations are pruned every day. The total number of
 * folders allowed in the Daily rotation folder (the folder where the
 * backupsets are written each day,) is limited to the number of
 * folders specified in the $this->app->localsys->rotation_total_daily
 * configuration parameter. The folders are sorted by creation date
 * and all but the most recent total_daily folders are deleted.
 *
 * <li>On one day of the week (specified in the
 * $this->app->localsys->rotation_dayofweek configuration parameter,)
 * the backupset that was just created is copied to the weekly backup
 * rotation. If the weekly rotation is stored on the same partition as
 * the daily rotation the copy will consist of hard linked files that
 * do not actually use any significant disk space.
 *
 * <li>Weekly rotations are pruned as with daily rotations. The
 * $this->app->localsys->rotation_total_weekly configuration parameter
 * defines the number of recent weekly backupsets that are maintained.
 *
 * <li>On one day of the month (specified in the
 * $this->app->localsys->rotation_dayofmonth configuration parameter,)
 * the backupset that was just created is copied to the monthly backup
 * rotation. This rotation can also be hardlinked to save disk space. 
 *
 * <li>Monthly rotations are pruned as with weekly rotations. The
 * $this->app->localsys->rotation_total_monthly configuration parameter
 * defines the number of recent monthly backupsets that are maintained.
 *
 * </ul>
 *
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


//require_once "lib/general-plugins.inc";


/**
 *
 * @package sysbackup
 * @subpackage sysbackup
 *
 */
class backupvfs_localio_rotation_plugin_c extends backupvfs_localio_c
{
   var $rotation_type     = "weekly"; // type is either weekly or monthly
   var $rotation_hardlink = true;     // true if daily & rotation partitions are the same


   function plugin_instance()  { return "{$this->rotation_type} Rotation on {$this->hostname}";   }
   function plugin_shortref()  { return "{$this->rotation_type}-rotation@" . $this->app->get_hostid(); }


   function __construct( &$application_object, &$tar_object, $type, $options = false )
   {
      parent::__construct( $application_object, $tar_object, $options );

      $this->rotation_type      = $type;
      $this->rotation_hardlink  = true;   // will verify later

      if ( ($this->rotation_type != "weekly")
        && ($this->rotation_type != "monthly") )
         $this->fatal( "rotation type must be weekly or monthly" );

      $this->plugin_enabled     = false;

      $this->plugin_type        = "vfslocalrotate";
      $this->plugin_unique      = true;

      $this->plugin_name        = "vfs-localio";
      $this->plugin_title       = "Local Fileio VFS Plugin";
      $this->plugin_version     = "0.01a";
      $this->plugin_description = "Plugin to write the backupset to the local filesystem";
   }



   function get_path_rotation()
   {
      switch ( $this->rotation_type )
      {
         case "weekly":
            return $this->app->get_path_weekly();

         case "monthly":
            return $this->app->get_path_monthly();

         default:
            break;
      }

      $this->fatal( "unknown rotation type: {$this->rotation_type}" );
   }



   function get_rotation_total()
   {
      switch ( $this->rotation_type )
      {
         case "weekly":
            return $this->app->get_rotations_weekly();

         case "monthly":
            return $this->app->get_rotations_monthly();

         default:
            break;
      }

      $this->fatal( "unknown rotation type: {$this->rotation_type}" );
   }



   function vfs_init( $backupset_name )
   {
      $this->backupset_name = $backupset_name;

      $properties_to_copy = array( "hostname"               => "get_hostname",
                                   "hostid"                 => "get_hostid",
                                   "path_temp"              => "get_path_temp",
                                   "path_daily"             => "get_path_daily",
                                   "path_weekly"            => "get_path_weekly",
                                   "path_monthly"           => "get_path_monthly",
                                   "umaskfolders"           => "get_umaskfolders",
                                   "umaskfiles"             => "get_umaskfiles",
                                   "usehardlinks"           => "get_usehardlinks",
                                   "minbackupsize"          => "get_minbackupsize",
                                   "minfreespace"           => "get_minfreespace",
                                   "rotation_enabled"       => "get_rotation_enabled",
                                   "rotation_dayofweek"     => "get_rotation_dayofweek",
                                   "rotation_dayofmonth"    => "get_rotation_dayofmonth",
                                   "rotation_total_daily"   => "get_rotations_daily",
                                   "rotation_total_weekly"  => "get_rotations_weekly",
                                   "rotation_total_monthly" => "get_rotations_monthly",
                                   );


      foreach ( $properties_to_copy as $p => $f )
         $this->$p = $this->app->$f();


      $this->plugin_enabled = false;

      if ( $this->rotation_type == "weekly" )
      {
         $today_dayofweek = date("w");
         $this->plugin_enabled  =  $this->rotation_enabled 
                               && ($this->rotation_dayofweek == $today_dayofweek);
      }


      if ( $this->rotation_type == "monthly" )
      {
         $today_dayofmonth = date("j");
         $this->plugin_enabled  =  $this->rotation_enabled 
                               && ($this->rotation_dayofmonth == $today_dayofmonth);
      }


      if ( $this->plugin_enabled )
      {
         $this->keeplocal          = true;
         $this->rotation_hardlink  = can_hardlink( $this->get_path_rotation(), $this->path_daily );

         if ( $this->vfs_warn_disk_space( true ) )
            $this->fatal( "problems found - please correct and try again" );

         return $this->vfs_createbackupset_folder( $this->get_path_rotation() );
      }

      // no problem but there's no work to be done today
      return false;
   }



   function vfs_start()
   {
      if ( !$this->plugin_enabled )
         return false;

      $this->_mark_backupset_incomplete();
      return $this->backupset->get_last_hashlist( $this->get_path_rotation() );
   }



   function vfs_end()
   {
      if ( !$this->plugin_enabled )
         return false;

      $this->backupset->current_hashlist->save_all();

      if ( $this->app->verbose() )
         $this->tee( "writing {$this->rotation_type} rotation last/prev backup names" );

      $this->backupset->save_last_backup_name( $this->get_path_rotation() );
      $this->_unmark_backupset_incomplete();

      return false;
   }



   function vfs_prune_expired_backups()
   {
      $description    = $this->get_hostname() . " {$this->rotation_type}";
      $path_to_prune  = $this->get_path_rotation();
      $max_items      = $this->get_rotation_total();

      return $this->_vfs_prune_expired_backups( $description, $path_to_prune, $max_items );
   }



   function rotation_copyfile( $rel_ref, $hash, $size )
   {
      if ( !$this->plugin_enabled )
         return false;


      $target = dir_fn( $this->get_path_backupset(), $rel_ref );
      $source = dir_fn( $this->app->get_path_backupset(), $rel_ref );

      $t = $this->vfs_displayref( $target );
      $s = $this->app->vfs_displayref( $source );

      if ( $this->rotation_hardlink )
      {
         // if this is a re-run the file might already exist
         if ( $this->_vfs_file_exists( $target ) )
            if ( $this->_vfs_unlink( $target ) === false )
               $this->fatal( "unable to unlink {$t}" );

         if ( $this->_vfs_link( $source, $target ) === false )
            $this->fatal( "unable to link {$t} to {$s}" );

         return false;
      }


      // find out if the file should be hardlinked somewhere?
      if ( ($where = $this->backupset->should_hardlink( $hash )) == false )
      {
         // nope - simple copy only
         if ( $this->app->verbose() )
            $this->tee( "Copying {$s} to {$t}" );

         if ( $this->_vfs_copy( $source, $target ) === false )
            $this->fatal( "unable to copy {$s} to {$t}" );

         if ( $this->_vfs_chmod( $target, $this->get_umaskfiles() ) === false )
            $this->fatal( "unable to change mode on file {$t}" );
      }


      $this->backupset->_register( $where, $rel_ref, $sha1, $size );
      return false;
   }
}



