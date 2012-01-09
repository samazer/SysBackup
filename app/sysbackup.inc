<?php /* -*- mode: php; mode: mmm; coding: utf-8-unix; -*- */
/**
 *
 * System Backups utility
 *
 * <p>This utility executes a full system backup.
 *
 * <p>The code to perform the various steps of the backup is found in
 * the plugins folder. The MySQL plugin, for example, is needed to
 * perform a backup of a MySQL server. The various jobs executed as
 * part of the backup process are listed in the configuration
 * folder. Each configuration file represents a backup job. Again, for
 * example, you can backup a MySQL server by copying the MySQL
 * configuration file into the configuration directory. Make two
 * copies of it to backup two MySQL servers. In each case the
 * information required for the job, such as server IP, backup user id
 * and password, is stored in the configuration file for the job.
 *
 * <p>Jobs are assigned to Events. When the event is triggered all the
 * jobs that have been assigned to it are sorted by Job ID (a user
 * defined string) and executed one-by-one. Each job is executed by
 * the plugin specified in the Job configuration.
 *
 * <p>There is a fixed list of events supported by this application:
 *
 * <ul>
 *
 * <li>sysreports    - System Status Reports
 *
 * <li>daemonsdown   - Daemon Shutdown
 *
 * <li>snapshotsup   - Partition Snapshot Create
 *
 * <li>daemonsup     - Daemon Restart
 *
 * <li>genbackups    - System Backup
 *
 * <li>snapshotsdown - Partition Snapshot close
 *
 * <li>auxops        - Auxiliary Operations Init (delayed copy to 
 *                     remote systems & exec of commands to sync 
 *                     backup archives on remote systems.)
 *
 * <li>summary       - Backup Summary Reports
 *
 * </ul>
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



include "lib/general-plugins.inc";
include "app/sysbackup-plugins.inc";
include "app/sysbackup-vfs.inc";



/**
 *
 * @package sysbackup
 * @subpackage sysbackup
 *
 */
class system_backup_app_c extends backupvfs_main_c
{
   function approve_job( $config_file, $job_info )
   {
      $required_options = array( 
            "id"          => array( true,  "the id assigned to this job" ),
            "rootreqd"    => array( false, "flag true if the app requires root "
                                         . "permissions to execute this job" ),
            "runonlyif"   => array( false, "string or array of conditions required "
                                         . "for running this job (plugin defined)" ),
                               );

      $job_approved = true;
      foreach ( $required_options as $opt => $info )
         if ( !isset( $job_info[$opt] ) )
         {
            if ( $info[0] )
            {
               $problems_found = true;
               $this->warning( "option '{$opt}' not specified in job id "
                             . "'{$job_info['id']}'" );
            }
         }
           else $$opt = $job_info[$opt];


      if ( isset( $job_info["rootreqd"] ) && $job_info["rootreqd"] )
         if ( $this->sys_user->user() != "root" )
            $this->fatal( "job id '{$id}' in '{$config_file}' requires "
                        . "root permissions" );



      if ( !isset( $job_info["runonlyif"] ) )
         return $job_approved;


      $runonly = $job_info["runonlyif"];
      if ( !is_array( $runonly ) )
         $this->fatal( "runonlyif in job '{$id}' in '{$config_file}' must "
                     . "be an array of folders that be found "
                     . "for this job to run" );

      foreach( $runonlyif as $cond => $folder )
      {
         if ( is_string( $cond ) )
            $this->fatal( "runonlyif in job '{$id}' in '{$config_file}' "
                        . "contains conditions that are not supported yet" );

         if ( !file_exists( $folder ) )
            $job_approved = false;
      }

      return $job_approved;
   }


   function app_before_init()
   {
      // see: sysbackup-vfs.inc: vfs_init()
      $this->vfs_init();

      $argv = &$GLOBALS["argv"];

      $this->hostname_short( $this->get_hostid() );
      $this->application_name( basename( $argv[0] ) );

      $this->teefn( dir_fn( $this->get_path_backupset(), 
                            $this->get_hostname() . EXT_OUTPUTCAPTURE ) );
  

      $cmdl = "";
      foreach( $argv as $arg )
         $cmdl = $cmdl
               . ($cmdl ? " " : "")
               . $arg
               ;

      $this->log( "[init] \"{$cmdl}\"" );
   }


   function app_title()
   {
      $t = $this->app_title . " " . $this->app_version;

      $this->log( $t );
      $this->tee( $t );
   }


   function app_after_init()
   {
      parent::app_after_init();

      $t = "Backupset Name is " . $this->get_backupset_name();
      $this->log( $t );
      if ( $this->verbose() )
         $this->tee( $t );
   }


   function after_loading_jobs()
   {
      $this->add_standard_internal_jobs();
   }


   function app_before_events()
   {
      $this->warn_disk_space( true );

      // see: sysbackup-vfs.inc: vfs_start()
      $this->vfs_start();
      $this->_notquiet_report_header(true);

      $this->max_job_subfolder_width = 0;
      foreach ( $this->events as $evt => $jobs )
         foreach ( $jobs as $job => $info )
            if ( isset( $info["subfolder"] ) )
               if ( ($w = strlen($info["subfolder"])) > $this->max_job_subfolder_width )
                  $this->max_job_subfolder_width = $w;
   }


   function app_after_events()
   {
      $this->summary_stats();

      // see: sysbackup-vfs.inc: vfs_end()
      $this->vfs_end();
   }


   function app_before_job( $event, $job_info )
   {
      // make sure we point to the main backupset folder
      // before each job to avoid having $targets going into
      // random subfolders.
      // see: sysbackup-vfs.inc: vfs_subfolder()
      $this->vfs_subfolder( "" );

      $this->warn_disk_space( false );

      $this->log( "@{$event}: {$job_info['jobindex']} {$job_info['plugin']}({$job_info['id']})" );
      $this->job_reset();
   }


   function app_after_job( $event, $job_info )
   {
      if ( $this->notquiet() )
         $this->show_stats( "job" );

      $this->log_stats( "job" );
   }


   function __construct( $opts = false )
   {
      $this->event_descriptions = array( "sysreports"    => "Generate System Reports",
                                         "daemonsdown"   => "Shutdown System Daemons",
                                         "snapshotsup"   => "Create LVM Snapshots",
                                         "daemonsup"     => "Restart System Daemons",
                                         "genbackups"    => "Generate the Backup Set",
                                         "snapshotsdown" => "Flush and destroy LVM Snapshots",
                                         "auxops"        => "Auxiliary Operations (ie: copy to NAS)",
                                         "summary"       => "Generate Summary Reports",
                                       );

      parent::__construct( $opts );
   }
}


