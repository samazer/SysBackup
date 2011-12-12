<?php /* -*- mode: php; mode: mmm; coding: utf-8-unix; -*- */
/**
 *
 * System utility Plugin Framework
 *
 * <p>This module contains a base class that can be sub-classed to
 * create a system utility with a plugin-based architecture.
 *
 * <p>This framework executes a system procedure in the following steps:
 *
 * <ul>
 *
 * <li>Load any plugins found. The system can list plugin files in a
 *     particular directory and load them into an appropriate list
 *     structure.
 *
 * <li>Load any configuration files found. Each configuration file is
 *     effectively an event handler that, when called, executes one of
 *     the plugins to perform a desired function. The plugin architecture
 *     will load all the configuration files, assign the various tasks to
 *     the desired events and sort them into the desired order.
 *
 * <li>Trigger the various events in the desired order, instantiating the
 *     appropriate plugin objects as needed for each task and executing
 *     the task.
 *
 * </ul>
 *
 * <p>The application will sub-class this framework and add any
 * additional behavior required. The actual tasks performed by the
 * application will then be controlled by the configuration scripts
 * which will make use of the various plugins.
 *
 *
 * <p><b>Copyright</b>
 *
 * <p>Copyright (c) 2001-2011 by Sam Azer, All Rights Reserved.
 *
 * <p>This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * <p>This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * <p>You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>
 *
 * <p><b>Copyright</b>
 *
 * <p>Copyright (c) 2001-2011 by Sam Azer, All Rights Reserved.
 *
 * <p>This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
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
 * @package aforms
 * @subpackage lib-general
 * @license http://www.gnu.org/licenses/agpl.html AGPL
 * @copyright 2011 by Sam Azer, All rights reserved
 * @link http://www.azertech.net/
 * @link http://www.samazer.net/about
 *
 */



define( "PLUGGABLEAPP_PLUGIN_FILEEXT", ".plugin" );
define( "PLUGGABLEAPP_CONFIG_FILEEXT", ".conf" );


require_once "general-tee.inc";
require_once "general-sysutils.inc";
require_once "general-sysexec.inc";
require_once "general-namedparameters.inc";


class plugin_base_c
{
   var $plugin_name;         //
   var $plugin_title;        //
   var $plugin_description;  //
   var $plugin_version;      // 

   var $plugin_enabled;      // false to prevent this plugin from being executed by default

   var $plugin_type;         // a type string to identify special plugins
   var $plugin_unique;       // true if only one plugin of this type is allowed

   var $app;                 // a reference to the application that loaded the plugin

   var $debug_sysexec;       // true to show exec/sysexec/sysfilter commands



   /**
    *
    * is Plugin Enabled?
    *
    * set or get the status of the plugin
    *
    */
   function plugin_enabled( $flg = -1 )
   {
      if ( $flg === -1 )
         return $this->plugin_enabled;

      return $this->plugin_enabled = $flg;
   }


   /**
    *
    * Plugin Instance
    *
    * return a short description string for the current instance of
    * the plugin
    *
    * This default method simply returns the plugin title - but each
    * instance of the plugin will have specific information. For
    * example a plugin that accesses a remote server will probably
    * return a description string that includes the remote server
    * name.
    *
    */
   function plugin_instance()
   {
      return $this->plugin_title . " " . $this->plugin_version;
   }


   /**
    *
    * Plugin Title
    *
    * Emit a title string for the plugin
    *
    */
   function plugin_title()
   {
      $this->app->tee( $this->plugin_title . " " . $this->plugin_version );
   }


   /**
    *
    * Plugin Help
    *
    * Emit a help message for the plugin
    *
    */
   function plugin_help()  
   {
      $this->app->tee( "(help for this plugin is not available yet)" );
   }


   /**
    *
    * Standard Warning
    *
    * Programmers helper: displays a message for methods that have not
    * been defined in the plugin code
    *
    */
   function _std_plugin_method_warning( $op )
   {
      $this->app->warning( "from plugin {$this->plugin_name}: {$op}" );
   }


   /**
    *
    * Required Options
    *
    * Programmers helper: returns an array of 
    *        Option => array( Required_flag, Help Text ), ...
    *
    */
   function required_options()
   {
      return array();
   }


   /**
    *
    * Plugin Load
    *
    * This method is called for ALL PLUGINS just before the
    * app_after_init() method is called (ie: almost at the end of the
    * init process.)
    *
    */
   function load()
   {
      $this->_std_plugin_method_warning( "dummy load operation" );
   }


   /**
    *
    * Plugin Unload
    *
    * This method is called for ALL PLUGINS just after the
    * app_after_events() method is called (ie: almost at the end of
    * app execution.)
    *
    */
   function unload()
   {
      $this->_std_plugin_method_warning( "dummy unload operation" );
   }


   /**
    *
    * Job Prepare
    *
    * This method is called for EACH JOB just after the
    * app_before_events() method is called (ie: just before any events
    * are triggered, before any jobs start. This is a good time to
    * emit any error messages that might be generated during job
    * execution.)
    *
    */
   function job_prepare( $configuration )
   {
      $this->_std_plugin_method_warning( "dummy job init" );
   }


   /**
    *
    * Job Exec
    *
    * This method is called for EACH JOB during the event to which the
    * job was assigned. Jobs are sorted by Job Index string and
    * executed in sorted order.
    *
    */
   function job_exec( $configuration )
   {
      $this->_std_plugin_method_warning( "dummy job exec" );
   }


   /**
    *
    * Job Cleanup
    *
    * This method is called for EACH JOB after all the jobs have been
    * exec'd and before the app_after_events() method is called. This
    * is a good time to perform any cleanup operations that might be
    * needed before the plugin is unloaded.
    *
    */
   function job_cleanup( $configuration )
   {
      $this->_std_plugin_method_warning( "dummy job cleanup" );
   }


   function set_debugsysexec( $flg = -1 )
   {
      if ( $flg === -1 )
         return $this->debug_sysexec;

      return $this->debug_sysexec = $flg;
   }


   function __construct( &$application_object )
   {
      $this->debug_sysexec      = false;

      $this->plugin_name        = "dummy";
      $this->plugin_title       = "Dummy Plugin";
      $this->plugin_version     = "0.00a";
      $this->plugin_description = "dummy plugin sample";
      $this->plugin_enabled     = false;
      $this->plugin_unique      = false;

      $this->app = &$application_object;
   }
}




class plugin_framework_c extends teelog_c
{
   var $app_title;
   var $app_version;
   var $app_conffile;
   var $app_confdir;
   var $app_plugindir;

   var $admin_email;

   var $verbose = false;  // flag true if debug info requested
   var $app_exec_flag = false; // true triggers events. See the --no-exec and --exec options
   var $app_exec_option = false; // true if the --no-exec or --exec option specified

   var $plugins;          // an array of plugins (names) that have been loaded.
   var $plugins_disabled; // an array of plugins that were not loaded because they were disabled.
   var $plugin_types;     // an array of plugin type strings [$plugin_types[$type] = array( $unique, $loaded_name )]

   var $plugin;           // an array of plugins (objects) that have been loaded.

   var $event_descriptions;// an array of event names and their descriptions for the current app
   var $events;           // an array of events, each of which contains an array of tasks

   var $current_event;    // the id of the currently executing event
   var $current_jobindex; // the index of the currently executing job
   var $current_job;      // a reference to the current job info array
   var $disabled_jobs;    // a list of disabled jobs (for reporting)

   var $progname;
   var $command;
   var $params;
   var $argv;

   var $named_parameters;
   var $np_default_index;
   var $np_default_method;


   var $debug_sysexec;   // true to debug any commands exec'd on the system



   function app_title()
   {
      $this->tee( $this->app_title . " " . $this->app_version );
   }


   function app_help()
   {
      $this->tee( "(help not available yet)" );
   }


   function dump_events()
   {
      $this->tee( "Event List:" );

      $width = 0;
      foreach ( $this->event_descriptions as $id => $desc )
         if ( ($w = strlen($id)) > $width )
            $width = $w;

      foreach ( $this->event_descriptions as $id => $desc )
      {
         $i = str_pad( $id, $width );
         $this->tee( "  {$i} {$desc}" );
      }

      $this->tee("");
   }


   function dump_jobs()
   {
      $this->tee( "Jobs Configured:" );

      $evt_width = $job_width = $info_width = 0;
      foreach ( $this->events as $evt => $jobs )
      {
         if ( ($w = strlen($evt)) > $evt_width )
            $evt_width = $w;

         foreach ( $jobs as $job => $info )
         {
            if ( ($w = strlen($job)) > $job_width )
               $job_width = $w;

            if ( ($w = strlen( "{$info['plugin']}({$info['id']})" )) > $info_width )
               $info_width = $w;
         }
      }

      $e = str_pad( "Event", $evt_width );
      $j = str_pad( "Index", $job_width );
      $i = str_pad( "Plugin(JobID)", $info_width );
      $this->tee( "  {$e} {$j} {$i} Description" );

      foreach ( $this->events as $evt => $jobs )
      {
         ksort( $jobs );
         foreach ( $jobs as $job => $info )
         {
            $e = str_pad( $evt, $evt_width );
            $j = str_pad( $job, $job_width );
            $i = str_pad( "{$info['plugin']}({$info['id']})", $info_width );
            $this->tee( "  {$e} {$j} {$i} {$info['description']}" );
         }
      }

      $this->tee("");
   }


   function dump_plugins()
   {
      $this->tee( "Plugins Loaded:" );

      foreach ( $this->plugins as $p )
      {
         $this->tee( "  " . sprintf( "%s %s (%s)", $this->plugin[$p]->plugin_title,
                                                   $this->plugin[$p]->plugin_version, $p ) );
         $this->tee( "    " . $this->plugin[$p]->plugin_description );
      }

      $this->tee("");

      $this->dump_jobs();

      if ( count( $this->plugins_disabled ) )
      {
         $this->tee( "Plugins Not Loaded (disabled):" );

         foreach ( $this->plugins_disabled as $p => $i)
            $this->tee( "  " .  sprintf( "%s %s (%s)", $this->plugins_disabled[$p]["plugin_title"],
                                                       $this->plugins_disabled[$p]["plugin_version"], $p ) );
      }

      $this->tee("");

      if ( count( $this->disabled_jobs ) )
      {
         $this->tee( "Jobs Not Configured:" );

         foreach ( $this->disabled_jobs as $j )
           $this->tee( "  " .  $j );
      }

      $this->tee("");
   }


   function dump_config()
   {
      $this->tee( "Basic Application Settings:" );

      $this->named_parameters->display_properties( $this, $this->np_default_method,
                                                          $this->np_default_index,
                                                          $this->debug_params_showhelp );

      $this->tee("");

      $this->dump_plugins();
   }



   function set_appexec( $flg = -1 )
   {
      if ( $flg === -1 )
         return $this->app_exec_flag;


      // option has been changed from the app default
      $this->app_exec_option = true; 
      return $this->app_exec_flag = $flg;
   }


   function set_verbose( $flg = -1 )
   {
      if ( $flg === -1 )
         return $this->verbose;

      return $this->verbose = $flg;
   }



   function set_debugsysexec( $flg = -1 )
   {
      if ( $flg === -1 )
         return $this->debug_sysexec;

      return $this->debug_sysexec = $flg;
   }



   function default_flag_action( $arg )
   {
      switch ( $arg )
      {
         case "-v":
         case "--verbose":
            $this->set_verbose(true);
            return true;

         case "-dx":
         case "--debug-sysexec":
            $this->set_debugsysexec(true);
            return true;

         case "--no-exec":
            $this->set_appexec(false);
            return true;

         case "--exec":
            $this->set_appexec(true);
            return true;


         default:
            // something not found
            return false;
      }

      return false;
   }



   function default_flag_args()
   {
      $arglist = $this->argv;
      foreach ( $arglist as $idx => $arg )
         $this->default_flag_action( $arg );
   }



   function default_action( $arg )
   {
      switch ( $arg )
      {
         case "-h":
         case "--help":
            $this->app_help();
            exit(0);

         case "--dump-config":
            $this->tee("");
            $this->dump_config();
            return true;

         case "--dump-plugins":
            $this->tee("");
            $this->dump_plugins();
            return true;

         case "--dump-jobs":
            $this->tee("");
            $this->dump_jobs();
            return true;

         case "--dump-events":
            $this->tee("");
            $this->dump_events();
            return true;

         case "--dump-config-help":
            $this->debug_params_showhelp = true;
            $this->tee("");
            $this->dump_config();
            return true;

         default:
            // something not found
            return false;
      }

      return false;
   }


   function default_action_args()
   {
      $arglist = $this->argv;
      foreach ( $arglist as $idx => $arg )
         if ( $this->default_action( $arg ) ) 
            unset( $this->argv[$idx] );
   }


   function add_plugin_disabled( &$obj )
   {
      $name   = $obj->plugin_name;

      foreach ( array( "plugin_name", 
                       "plugin_title", 
                       "plugin_version", 
                       "plugin_description",
                       "plugin_type",
                       "plugin_unique",
                       ) as $i )
         $this->plugins_disabled[$name][$i] = $obj->$i;
   }


   function add_plugin( &$obj )
   {
      $name   = $obj->plugin_name;
      $type   = $obj->plugin_type;
      $unique = $obj->plugin_unique;

      if ( $obj->plugin_enabled )
      {
         if ( isset( $obj->plugin_rootreqd ) && $obj->plugin_rootreqd )
            if ( $this->sys_user->user() != "root" )
               $this->fatal( "plugin '{$name}' requires root permissions" );


         if ( isset( $this->plugin[$name] ) )
         {
            $this->warning( "Trying to load plugin '{$name}'" );
            $this->warning( "Another plugin with the same name is already loaded:" );
            $this->warning( "{$name}: {$this->plugin[$name]->plugin_description} ({$this->plugin[$name]->plugin_version})" );

            $this->fatal( "unable to replace plugin \"{$name}\" with another plugin using the same name" );
         }

         if ( isset($this->plugin_types[$type]) )
         {
            if ( $this->plugin_types[$type]['unique'] || $unique )
            {
               $this->warning( "Only one plugin of type '{$type}' can be loaded." );
               $this->fatal( "Plugin '{$name}' of type '{$type}' "
                           . "can't replace plugin '{$this->plugin_types[$type]['name'][0]}'" );
            }

            if ( !$unique && !$this->plugin_types[$type]['unique'] )
               $this->plugin_types[$type]['name'][] = $name;
         }
           else
         {
            $this->plugin_types[$type]['unique'] = $unique;
            $this->plugin_types[$type]['name'][] = $name;
         }

         $this->plugins[]     = $name;
         $this->plugin[$name] = &$obj;
      }
        else $this->add_plugin_disabled( $obj );
   }


   function load_app_config()
   {
      if ( $this->verbose )
         $this->tee( "loading app config \"{$this->app_conffile}\"" );

      if ( !is_readable( $this->app_conffile ) )
         $this->fatal( "unable to read \"{$this->app_conffile}\"" );

      include $this->app_conffile;
   }


   function load_plugins()
   {
      if ( $this->verbose )
         $this->tee( "loading plugins from \"{$this->app_plugindir}\"" );

      if ( !file_exists( $this->app_plugindir ) )
         $this->fatal( "can't find plugin folder \"{$this->app_plugindir}\"" );

      if ( chdir( $this->app_plugindir ) === false )
         $this->fatal( "unable to change to \"{$this->app_plugindir}\"" );

      $list = glob( "*" . PLUGGABLEAPP_PLUGIN_FILEEXT );
      foreach ( $list as $file )
      {
         $plugin_file = str_replace( "//", "/", $this->app_plugindir . "/" . $file );

         if ( $this->verbose )
           $this->tee( "  loading plugin: {$file}" );

         include $plugin_file;
      }

      $c = count($list) - (count($this->plugins) + count($this->plugins_disabled));
      if ( $c > 0 )
      {
         $c = $c . " plugin(s) not loaded for unknown reason(s)";

         if ( $this->verbose )
            $this->fatal( $c );
         else $this->warning( $c );
      }
   }


   function approve_job( $config_file, $job_info )
   {
      return true; // approve by default
   }


   function add_job( $config_file, $job_info )
   {
      $required_options = array( 
         "id"          => array( true, "the id assigned to this job" ),
         "description" => array( true, "the description for this job" ),

         "enabled"     => array( true, "true if the job is enabled, false to skip it" ),

         "plugin"      => array( true,  "the name of the plugin to use for this job" ),
         "method"      => array( false, "method name to exec for internal jobs performed by methods in the main application object" ),

         "event"       => array( true, "the event to which this job is assigned" ),
         "jobindex"    => array( true, "the order in which this job will be called within the event" ),
                                       );

      $problems_found = false;
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

      if ( isset( $job_info["plugin"] ) 
        && ($job_info["plugin"] > "") 
        && ($job_info["plugin"] != "internal") 
         )
         if ( isset( $this->plugin[$plugin] ) )
         {
            $required_options = $this->plugin[$plugin]->required_options();

            foreach ( $required_options as $opt => $info )
               if ( !isset( $job_info[$opt] ) && $info[0] )
               {
                  $problems_found = true;
                  $this->warning( "option '{$opt}' not specified in job id "
                                . "'{$job_info['id']}" );
               }
         }

      if ( $problems_found )
         $this->fatal( "job id '{$job_info['id']}' configuration error; "
                     . "please fix file '{$config_file}' and try again" );

      if ( $job_info["enabled"] )
      {
         if ( ($plugin != "internal") && !isset( $this->plugin[$plugin] ) )
            $this->fatal( "job id '{$id}' in '{$config_file}' refers to "
                        . "plugin '{$plugin}' which is not loaded" );

         if ( !isset( $this->events[$event] ) )
            $this->fatal( "job id '{$id}' in '{$config_file}' refers to "
                        . "event '{$event}' which is not defined" );

         if ( ($job_approved = $this->approve_job( $config_file, $job_info )) != false )
         {
            // fix the job index in cases where there's a collision with another job
            if ( isset( $this->events[$event][$jobindex] ) )
            {
               $ji = $jobindex;
               if ( substr( $ji, -5 ) == ".0000" )
                  $ji = substr( $jobindex, 0, -5 );

               $counter = 0;
               $job_str = $ji . ".0000";
               while ( isset( $this->events[$event][$job_str] ) )
               {
                  $counter++;
                  $job_str = $ji
                           . ($counter ? ("." . substr( "0000" . $counter, -4 ))
                                       : "")
                           ;
               } 

               $jobindex = $job_str;
            }

            if ( isset( $this->events[$event][$jobindex] ) )
               $this->fatal( "code error - unexpected job index collision" );

            $this->events[$event][$jobindex] = $job_info;

            if ( $this->verbose )
               $this->tee( "    Adding {$id} to {$event}:{$jobindex} running {$plugin}" );

            return false;
         }
      }

      $dis = "{$id} in '{$config_file}' "
                  . ($job_approved ? "(disabled)" 
                                   : "(disabled by run conditions)" );

      $this->disabled_jobs[] = $dis;

      if ( $this->verbose )
         $this->tee( "    Skipping job " . $dis );

      return false;
   }


   function load_jobs()
   {
      if ( $this->verbose )
         $this->tee( "loading jobs from \"{$this->app_confdir}\"" );

      if ( chdir( $this->app_confdir ) === false )
         $this->fatal( "unable to change to \"{$this->app_confdir}\"" );

      $list = glob( "*" . PLUGGABLEAPP_CONFIG_FILEEXT );

      foreach ( $list as $file )
      {
         $config_file = str_replace( "//", "/", $this->app_confdir . "/" . $file );

         if ( $this->verbose )
           $this->tee( "  loading job: {$config_file}" );

         include $config_file;
      }
   }


   function app_before_init()
   {
      // do nothing here
   }


   function app_after_init()
   {
      // do nothing here
   }


   function before_loading_plugins()
   {
      // do nothing here
   }


   function before_loading_jobs()
   {
      // do nothing here
   }


   function after_loading_jobs()
   {
      // do nothing here
   }


   function app_init()
   {
      $this->load_app_config();
      $this->app_before_init();

      $this->default_flag_args();

      $this->app_title();

      if ( $this->verbose )
         $this->tee( "initializing..." );

      $this->before_loading_plugins();
      $this->load_plugins();
      $this->before_loading_jobs();
      $this->load_jobs();
      $this->after_loading_jobs();

      if ( count( $this->argv ) == 1 )
      {
         $this->app_help();
         exit(0);
      }

      // tell all the plugins that they've been loaded
      if ( $this->app_exec_flag )
      {
         if ( $this->verbose )
            $this->tee( "...loading plugins at " . $this->all_time(true) . " seconds" );

         foreach ( $this->plugins as $p )
            $this->plugin[$p]->load();
      }

      $this->app_after_init();

      if ( $this->verbose )
         $this->tee( "...finished initializing" );
   }


   function app_before_events()
   {
      // do nothing here
   }


   function app_after_events()
   {
      // do nothing here
   }


   function app_before_job( $event, $job_info )
   {
      // do nothing here
   }


   function app_after_job( $event, $job_info )
   {
      // do nothing here
   }


   function trigger_events()
   {
      if ( $this->verbose )
         $this->tee( "preparing jobs at " . $this->all_time(true) . " seconds" );

      foreach ( $this->events as $evt => $jobs )
      {
         ksort( $jobs );
         foreach ( $jobs as $job => $info )
            if ( $info['plugin'] != "internal" )
               $this->plugin[$info['plugin']]->job_prepare( $info );
      }

      foreach ( $this->events as $ev => $jobs )
      {
         if ( $this->verbose )
            $this->tee( "Triggering Event \"{$this->event_descriptions[$ev]}\" ({$ev})" );

         $this->current_event = $ev;

         ksort( $jobs );

         foreach ( $jobs as $idx => $job )
         {
            if ( $this->verbose )
               $this->tee( "  Starting job \"{$idx}\"" );

            $this->current_jobindex = $idx;
            $this->current_job      = &$job;

            $this->app_before_job( $ev, $job );

            if ( $job['plugin'] != "internal" )
               $this->plugin[$job['plugin']]->job_exec( $job );
            else $this->$job['method']();

            $this->app_after_job( $ev, $job );
         }
      }

      if ( $this->verbose )
         $this->tee( "job cleanup starts at " . $this->all_time(true) . " seconds" );

      foreach ( $this->events as $evt => $jobs )
      {
         ksort( $jobs );
         foreach ( $jobs as $job => $info )
            if ( $info['plugin'] != "internal" )
               $this->plugin[$info['plugin']]->job_cleanup( $info );
      }
   }


   function main()
   {
      $this->all_start(true);

      $this->app_init();

      if ( $this->verbose )
         $this->tee( "Execution begins at " . $this->all_time(true) . " seconds" );

      $this->app_before_events();
      $this->default_action_args();

          if ( $this->app_exec_flag )
          {
             $this->trigger_events();

             $this->app_after_events();

             // tell all the plugins that they're about to be unloaded
             if ( $this->verbose )
                $this->tee( "unloading plugins at " . $this->all_time(true) . " seconds" );

             foreach ( $this->plugins as $p )
                $this->plugin[$p]->unload();
          }
      elseif ( !$this->app_exec_option )
             $this->tee( "[app exec disabled; see the --exec and --no-exec options]" );

      $this->all_stop(true);

      if ( $this->verbose )
         $this->tee( "...exec ends after " . $this->all_time(true) . " seconds" );
   }



   function __construct( $initial_config = false )
   {
      parent::__construct();

      $this->debug_sysexec    = false;  // true to show sysexec/sysfilter commands

      $this->named_parameters = new named_parameters_c( "apps", "general-plugins" );
      $this->named_parameters->set_logger( $this );

      $np_method = $this->np_default_method = "general-plugin-support";
      $np_index  = $this->np_default_index  = "base";

      $this->named_parameters->register( $np_method, $np_index, array( 
            "app_title"     => array( 3, PLUGGABLEAPP_TITLE,      "the title of this application"         ),
            "app_version"   => array( 3, PLUGGABLEAPP_VERSION,    "current version of this application"   ),
            "app_conffile"  => array( 1, PLUGGABLEAPP_CONF,       "path/name of the main app config file" ),
            "app_confdir"   => array( 1, PLUGGABLEAPP_CONF_D,     "path to the main app config dir"       ),
            "app_plugindir" => array( 1, PLUGGABLEAPP_PLUGIN_D,   "path to the main app plugin dir"       ),
            "admin_email"   => array( 1, PLUGGABLEAPP_ADMINEMAIL, "email address for app administrator"   ),
                                                 ));

      $this->plugins = array();
      $argv = $this->argv = $GLOBALS["argv"];

      $this->command  = "";
      $this->progname = $argv[0];

      if ( count($this->event_descriptions) 
        && ($this->events == NULL)  
         )
         foreach( $this->event_descriptions as $ev => $help )
            $this->events[$ev] = array();

      if ( $initial_config === false )
         $initial_config = array();

      $this->named_parameters->set_optional( $np_method, $np_index, $initial_config );
      $this->named_parameters->check( $np_method, $np_index, $initial_config );
      $this->named_parameters->set_properties( $this, $np_method, $np_index, $initial_config );
   }
}



