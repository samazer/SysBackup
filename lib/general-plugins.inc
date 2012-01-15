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
 * @subpackage lib-sys
 * @license http://www.gnu.org/licenses/agpl.html AGPL
 * @copyright 2011 by Sam Azer, All rights reserved
 * @link http://www.azertech.net/
 * @link http://www.samazer.net/about
 *
 */



define( "PLUGGABLEAPP_PLUGIN_FILEEXT", ".plugin" );
define( "PLUGGABLEAPP_CONFIG_FILEEXT", ".conf" );
define( "PLUGGABLEAPP_PLUGIN_INTERNAL", "internal" );



require_once "general-tee.inc";
require_once "general-sysutils.inc";
require_once "general-sysexec.inc";
require_once "general-namedparameters.inc";
require_once "general-args.inc";


/**
 * 
 * Plugin APP class
 *
 * A small utility to take care of a reference to the main application.
 *
 */
class plugin_appreference_c
{
   var $app;          // reference to the main app object (ie: to access the log and error objects)

   function __construct( &$application_object )
   {
      $this->app = &$application_object;
   }


   function _message_prefix_text()
   {
      return $this->plugin_shortref() . ": ";
   }


   function log( $msg, $eol = "\n" )
   {
      $this->app->log( $this->_message_prefix_text() . $msg, $eol );
   }


   function tee( $msg, $eol = "\n" )
   {
      $this->app->tee( $this->_message_prefix_text() . $msg, $eol );
   }


   function warning( $msg )
   {
      $this->app->warning( $this->_message_prefix_text() . $msg );
   }


   function fatal( $msg, $stop_flag = true )
   {
      $this->app->fatal( $this->_message_prefix_text() . $msg, $stop_flag );
   }
}



/**
 *
 * @package aforms
 * @subpackage lib-sys
 *
 */
class plugin_base_c extends plugin_appreference_c
{
   var $plugin_name;         //
   var $plugin_title;        //
   var $plugin_description;  //
   var $plugin_version;      // 

   var $plugin_enabled;      // false to prevent this plugin from being executed by default

   var $plugin_type;         // a type string to identify special plugins
   var $plugin_unique;       // true if only one plugin of this type is allowed

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
   * A shorter version of the instance string
   *
   */
   function plugin_shortref()
   {
      return $this->plugin_name;
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
      parent::__construct( $application_object );

      $this->debug_sysexec      = false;

      $this->plugin_name        = "dummy";
      $this->plugin_title       = "Dummy Plugin";
      $this->plugin_version     = "0.00a";
      $this->plugin_description = "dummy plugin sample";
      $this->plugin_enabled     = false;
      $this->plugin_unique      = false;
   }
}




/**
 *
 * @package aforms
 * @subpackage lib-sys
 *
 */
class plugin_framework_c extends teelog_c
{
   private $sys_timezone;        // default timezone to keep PHP happy

   var $app_title;
   var $app_version;
   var $app_conffile;
   var $app_confdir;
   var $app_plugindir;

   var $admin_email;

   var $verbose_flag  = false; // flag true if debug info requested
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



   function set_adminemail( $e )       { $this->admin_email  = $e;              }
   function set_timezone( $t )         { $this->sys_timezone = $t;              }

   function get_adminemail()           { return $this->admin_email;             }
   function get_timezone()             { return $this->sys_timezone;            }

   function set_dir_config( $d )       { $this->app_confdir  = $d;              }
   function set_dir_plugins( $d )      { $this->app_plugindir = $d;             }

   function get_dir_plugins()          { return $this->app_confdir;             }
   function get_dir_config()           { return $this->app_confdir;             }

   function app_title()
   {
      $this->tee( $this->app_title . " " . $this->app_version );
   }


   function app_commandline_help()
   {
      $this->tee( "Recognized Command Line Args include:" );
      $this->tee("");

      capture_begin();
      general_args_c::show();
      $r = explode( "\n", capture_end() );

      foreach ( $r as $t )
         $this->tee( "   " . $t );
   }


   function app_help()
   {
      $this->tee( "(help not available yet)" );
      $this->tee("");
      $this->app_commandline_help();

      exit(0);
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



   function appmode_sanitycheck()
   {
      return !$this->app_exec_flag;
   }


   function appmode_exec()
   {
      return $this->app_exec_flag;
   }



   function set_appexec( $flg = -1 )
   {
      if ( $flg === -1 )
         return $this->app_exec_flag;


      // option has been changed from the app default
      $this->app_exec_option = true; 
      return $this->app_exec_flag = $flg;
   }


   function verbose( $flg = -1 )
   {
      if ( $flg === -1 )
         return $this->verbose_flag;

      return $this->verbose_flag = $flg;
   }



   function set_debugsysexec( $flg = -1 )
   {
      if ( $flg === -1 )
         return $this->debug_sysexec;

      return $this->debug_sysexec = $flg;
   }



   function default_action( $arg )
   {
      switch ( $arg )
      {
         case "-h":
         case "--help":
            $this->app_help();
            return true;

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
      $arglist = general_args_c::get_arg( "general-plugins" );

      foreach ( $arglist as $idx => $arg_info )
         $this->default_action( $arg_info[3][0] );
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
      $this->fatal( "the internal plugin has not been enabled" );
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


      if ( $name === PLUGGABLEAPP_PLUGIN_INTERNAL )
         $this->fatal( "plugins with the name '" . PLUGGABLEAPP_PLUGIN_INTERNAL . "' are not allowed" );


      if ( $obj->plugin_enabled )
         if ( isset( $obj->plugin_rootreqd ) && $obj->plugin_rootreqd )
            if ( $this->sys_user->user() != "root" )
            {
               $this->warning( "plugin '{$name}' requires root permissions" );
               $this->add_plugin_disabled( $obj );
               return;
            }


      if ( $obj->plugin_enabled )
      {
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
      if ( $this->verbose() )
         $this->tee( "loading app config \"{$this->app_conffile}\"" );

      if ( !is_readable( $this->app_conffile ) )
         $this->fatal( "unable to read \"{$this->app_conffile}\"" );

      include $this->app_conffile;
   }


   function load_plugins()
   {
      if ( $this->verbose() )
         $this->tee( "loading plugins from \"{$this->app_plugindir}\"" );

      if ( !file_exists( $this->app_plugindir ) )
         $this->fatal( "can't find plugin folder \"{$this->app_plugindir}\"" );

      if ( chdir( $this->app_plugindir ) === false )
         $this->fatal( "unable to change to \"{$this->app_plugindir}\"" );

      $list = glob( "*" . PLUGGABLEAPP_PLUGIN_FILEEXT );
      foreach ( $list as $file )
      {
         $plugin_file = str_replace( "//", "/", $this->app_plugindir . "/" . $file );

         if ( $this->verbose() )
           $this->tee( "  loading plugin: {$file}" );

         include $plugin_file;
      }

      $c = count($list) - (count($this->plugins) + count($this->plugins_disabled));
      if ( $c > 0 )
      {
         $c = $c . " plugin(s) not loaded for unknown reason(s)";

         if ( $this->verbose() )
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
        && ($job_info["plugin"] != PLUGGABLEAPP_PLUGIN_INTERNAL) 
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

      if ( $job_info["enabled"] 
        && ($plugin != PLUGGABLEAPP_PLUGIN_INTERNAL) && !isset( $this->plugin[$plugin] ) )
      {
         $problems_found = true;
         $this->warning( "job id '{$id}' in '{$config_file}' refers to "
                       . "plugin '{$plugin}' which is not loaded" );
      }

      if ( $job_info["enabled"] && !isset( $this->events[$event] ) )
      {
         $problems_found = true;
         $this->warning( "job id '{$id}' in '{$config_file}' refers to "
                       . "event '{$event}' which is not defined" );
      }


     /**
      * TODO: the next three lines have no meaning because the
      * approve_job() method is a stub and always approves a job. As a
      * result the disabled message below needs to be re-written when
      * the approve_job() stub is replaced with the real thing.
      */
      $job_approved = true;
      if ( !$problems_found && $job_info["enabled"] )
         $job_approved = $this->approve_job( $config_file, $job_info );


      if ( $problems_found || !$job_info["enabled"] || !$job_approved )
      {
         if ( $problems_found )
         {
            $dis = "job id '{$job_info['id']}' (problem(s) found; disabled)";
            $this->warning( $dis );
         }
           else $dis = "{$id} in '{$config_file}' "
                     . ($job_approved ? "(disabled)" 
                                      : "(disabled by run conditions)" );

         $this->disabled_jobs[] = $dis;

         if ( $this->verbose() )
            $this->tee( "    Skipping job " . $dis );

         return false;
      }

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

      if ( $this->verbose() )
         $this->tee( "    Adding {$id} to {$event}:{$jobindex} running {$plugin}" );

      return false;
   }


   function load_jobs()
   {
      if ( $this->verbose() )
         $this->tee( "loading jobs from \"{$this->app_confdir}\"" );

      if ( chdir( $this->app_confdir ) === false )
         $this->fatal( "unable to change to \"{$this->app_confdir}\"" );

      $list = glob( "*" . PLUGGABLEAPP_CONFIG_FILEEXT );

      foreach ( $list as $file )
      {
         $config_file = str_replace( "//", "/", $this->app_confdir . "/" . $file );

         if ( $this->verbose() )
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
      general_args_c::scan();

      if ( general_args_c::display_errors() )
      {
         echo "stopping\n";
         exit(1);
      }

      $this->load_app_config();
      $this->app_before_init();

      if ( $this->verbose() )
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
      if ( $this->appmode_exec() )
      {
         if ( $this->verbose() )
            $this->tee( "...loading plugins at " . $this->all_time(true) . " seconds" );

         foreach ( $this->plugins as $p )
            $this->plugin[$p]->load();
      }

      $this->app_after_init();

      if ( $this->verbose() )
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
      if ( $this->verbose() )
         $this->tee( "preparing jobs at " . $this->all_time(true) . " seconds" );

      foreach ( $this->events as $evt => $jobs )
      {
         ksort( $jobs );
         foreach ( $jobs as $job => $info )
            if ( $info['plugin'] != PLUGGABLEAPP_PLUGIN_INTERNAL )
               $this->plugin[$info['plugin']]->job_prepare( $info );
      }

      foreach ( $this->events as $ev => $jobs )
      {
         if ( $this->verbose() )
            $this->tee( "Triggering Event \"{$this->event_descriptions[$ev]}\" ({$ev})" );

         $this->current_event = $ev;

         ksort( $jobs );

         foreach ( $jobs as $idx => $job )
         {
            if ( $this->verbose() )
               $this->tee( "  Starting job \"{$idx}\"" );

            $this->current_jobindex = $idx;
            $this->current_job      = &$job;

            $this->app_before_job( $ev, $job );

            if ( $job['plugin'] != PLUGGABLEAPP_PLUGIN_INTERNAL )
               $this->plugin[$job['plugin']]->job_exec( $job );
            else $this->plugin_internal($job);

            $this->app_after_job( $ev, $job );
         }
      }

      if ( $this->verbose() )
         $this->tee( "job cleanup starts at " . $this->all_time(true) . " seconds" );

      foreach ( $this->events as $evt => $jobs )
      {
         ksort( $jobs );
         foreach ( $jobs as $job => $info )
            if ( $info['plugin'] != PLUGGABLEAPP_PLUGIN_INTERNAL )
               $this->plugin[$info['plugin']]->job_cleanup( $info );
      }
   }


   function main()
   {
      $this->all_start(true);

      $this->app_title();

      $this->app_init();

      if ( $this->verbose() )
         $this->tee( "Execution begins at " . $this->all_time(true) . " seconds" );

      $this->app_before_events();
      $this->default_action_args();

          if ( $this->appmode_exec() )
          {
             $this->trigger_events();

             $this->app_after_events();

             // tell all the plugins that they're about to be unloaded
             if ( $this->verbose() )
                $this->tee( "unloading plugins at " . $this->all_time(true) . " seconds" );

             foreach ( $this->plugins as $p )
                $this->plugin[$p]->unload();
          }
      elseif ( !$this->app_exec_option )
             $this->tee( "[app exec disabled; see the --exec and --no-exec options]" );

      $this->all_stop(true);

      if ( $this->verbose() )
         $this->tee( "...exec ends after " . $this->all_time(true) . " seconds" );
   }



   function commandline_args_callback( $group_id,
                                       $arg_id, $param_count, $args,
                                       $arg_spec
                                     )
   {
      switch( $arg_id )
      {
         case "config":
         case "confdir":
         case "plugindir":

            $p = $arg_spec["prop"][0];
            $this->$p = $args[1];
            break;

         case "verbose":
            $this->verbose(true);
            break;

         case "debugsysexec":
            $this->set_debugsysexec(true);
            break;

         case "no-exec":
            $this->set_appexec(false);
            break;

         case "exec":
            $this->set_appexec(true);
            return true;

         case "help":
         case "dump-config":
         case "dump-plugins":
         case "dump-jobs":
         case "dump-events":
         case "dump-config-help":
            break;

         default:
            $this->warning( "unknown arg id: {$arg_id}" );
            break;
      }
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


      $command_line_args = array(
         "config" => array( "args"    => array( "-c", "--config", "--conf" ),
                            "params"  => 1,
                            "prop"    => array( "app_conffile" ),
                            "test"    => array( "file-exists" ),
                            "default" => array( PLUGGABLEAPP_CONF ),
                            "example" => "--config <file>",
                            "desc"    => "Set the path and file name of the sysbackup configuration file.",
                          ),

        "confdir" => array( "args"    => array( "-d", "--conf.d", "--confdir" ),
                            "params"  => 1,
                            "prop"    => "app_confdir",
                            "default" => PLUGGABLEAPP_CONF_D,
                            "example" => "--confdir <path>",
                            "desc"    => "Specify the directory that holds the job configuration files",
                          ),

      "plugindir" => array( "args"    => array( "-p", "--plugin.d", "--plugindir" ),
                            "params"  => 1,
                            "prop"    => "app_plugindir",
                            "default" => PLUGGABLEAPP_PLUGIN_D,
                            "example" => "--plugindir <path>",
                            "desc"    => "Specify the directory that holds the plugin scripts",
                          ),

        "verbose" => array( "args"    => array( "-v", "--verbose" ),
                            "params"  => 0,
                            "desc"    => "Enable the display of detailed activity information",
                          ),

   "debugsysexec" => array( "args"    => array( "-dx", "--debug-sysexec" ),
                            "params"  => 0,
                            "desc"    => "Display system commands executed by the application",
                          ),

        "no-exec" => array( "args"    => array( "--no-exec", "--dry-run", "--sanity-check" ),
                            "params"  => 0,
                            "desc"    => "Perform various sanity checks and report any problems found - do not execute",
                          ),

           "exec" => array( "args"    => array( "--exec" ),
                            "params"  => 0,
                            "desc"    => "Instruct the application to execute fully",
                          ),

           "help" => array( "args"    => array( "-h", "--help" ),
                            "params"  => 0,
                            "desc"    => "Display the internal application help text",
                          ),

    "dump-config" => array( "args"    => array( "--dump-config" ),
                            "params"  => 0,
                            "desc"    => "Load the configuration and display it",
                          ),

"dump-config-help"=> array( "args"    => array( "--dump-config-help" ),
                            "params"  => 0,
                            "desc"    => "Load the configuration and display it - including any available help texts",
                          ),

   "dump-plugins" => array( "args"    => array( "--dump-plugins" ),
                            "params"  => 0,
                            "desc"    => "Load the configuration and display the plugin list",
                          ),

    "dump-events" => array( "args"    => array( "--dump-events" ),
                            "params"  => 0,
                            "desc"    => "Load the configuration and display the event list",
                          ),
                                 );


      general_args_c::register( "general-plugins", 
                                "Config", "Configuration options",
                                $command_line_args );

      general_args_c::set_callback( "general-plugins", 
                                    $this, 
                                    "commandline_args_callback" );

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



