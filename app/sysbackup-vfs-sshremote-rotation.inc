<?php /* -*- mode: php; mode: mmm; coding: utf-8-unix; -*- */
/**
 *
 * Rotation Plugin - sshremote storage
 *
 * Note that these rotation plugins are not actually loaded as plugins
 * or as virtual filesystems. These are objects that interact with the
 * VFS objects.
 *
 * See the notes in the header of the localio rotation plugin.
 * This plugin is intended to be identical but operates
 * on a remote filesystem.
 *
 * This plugin is called after a file has been copied to the ssh
 * remote NAS server. It is intended to determine how best to create a
 * weekly or monthly rotation. Each rotation is actually a separate
 * backupset in the case where hardlinks are not available - so the
 * code operates as if it is creating a new backupset. 
 *
 * The situation is quite simple if it is possible to use hardlinks
 * between the daily rotations and the weekly rotations. In this case
 * the necessary folders are created and hardlinks are written as the
 * backup progresses.
 *
 * Otherwise the rotation plugin loads the hashlist for the previously
 * generated backupset in the rotation. It attempts to save disk space
 * via hardlinks between files in the new weekly backupset and the
 * previous weekly backupset using the same logic as the local I/O
 * backup plugin. If no identical file is found the remote server is
 * instructed to make a copy of the file from the daily rotation to
 * the weekly or monthly backupset.
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
 * @package sysbackup
 * @subpackage sysbackup
 * @license http://www.gnu.org/licenses/agpl.html AGPL
 * @copyright 2011 by Sam Azer, All rights reserved
 * @link http://www.azertech.net/
 * @link http://www.samazer.net/about
 *
 */


//require_once "lib/general-plugins.inc";


class backupvfs_sshremote_rotation_c extends backupvfs_sshremote_c
{
   var $vfs;                          // a reference to the sshremote backup object
   var $rotation_type     = "weekly"; // type is either weekly or monthly
   var $rotation_hardlink = true;     // true if daily & rotation partitions are the same


   function plugin_instance()  { return "Remote {$this->rotation_type} Rotation on {$this->remote->remote_server}";   }
   function plugin_shortref()  { return "ssh-{$this->rotation_type}-rotation@" . $this->remote->remote_server; }


   function get_path_rotation()
   {
      switch ( $this->rotation_type )
      {
         case "weekly":
            return $this->vfs->get_path_weekly();

         case "monthly":
            return $this->vfs->get_path_monthly();

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



   function __construct( &$application_object, &$vfs_object, $type, $options = false )
   {
      parent::__construct( $application_object, $options );

      $this->vfs                = &$vfs_object;
      $this->rotation_type      = $type;
      $this->rotation_hardlink  = true;   // will verify later

      if ( ($this->rotation_type != "weekly")
        && ($this->rotation_type != "monthly") )
         $this->fatal( "rotation type must be weekly or monthly" );

      $this->plugin_enabled     = false;

      $this->plugin_type        = "sshremoterotate";
      $this->plugin_unique      = true;

      $this->plugin_name        = "vfs-sshremote-rotate";
      $this->plugin_title       = "SSHRemote Rotate VFS Plugin";
      $this->plugin_version     = "0.01a";
      $this->plugin_description = "Plugin to copy a backupset to a weekly or monthly rotation on a remote filesystem";
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
         $this->$p = $this->vfs->$f();


      $this->plugin_enabled = false;

      if ( $this->rotation_type == "weekly" )
      {
         $today_dayofweek = date("w");
         $this->plugin_enabled  =  $this->rotation_enabled 
                               &&  $this->rotation_total_weekly
                               && ($this->rotation_dayofweek == $today_dayofweek);
      }


      if ( $this->rotation_type == "monthly" )
      {
         $today_dayofmonth = date("j");
         $this->plugin_enabled  =  $this->rotation_enabled 
                               &&  $this->rotation_total_monthly
                               && ($this->rotation_dayofmonth == $today_dayofmonth);
      }


      if ( $this->plugin_enabled )
      {
         $this->keeplocal          = true;
         $this->rotation_hardlink  = $this->_vfs_can_hardlink( $this->get_path_rotation(), $this->path_daily );

         $this->load_remote_configuration_file();

         $problems_found = $this->vfs_warn_disk_space( true );
         if ( !$this->app->get_keeplocal() && $problems_found )
            $this->fatal( "problems found - please correct and try again" );

         if ( $this->app->appmode_exec() )
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
      $source = dir_fn( $this->vfs->get_path_backupset(), $rel_ref );

      $t = $this->vfs_displayref( $target );
      $s = $this->vfs_displayref( $source );

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

