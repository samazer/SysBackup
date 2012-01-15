<?php /* -*- mode: php; mode: mmm; coding: utf-8-unix; -*- */
/**
 *
 * Application Command Line Arguments handling
 *
 * The basic issue is that different classes handle different command
 * line arguments. The classes are often in different modules and it
 * is not possible, currently, to gather them all together for
 * purposes such as reporting invalid args.
 *
 * This class will allow all the different modules to register the
 * arguments that they recognize. The arguments will be registered
 * with enough information that the class will be able to (at the very
 * least) perform the most basic checks for validity.
 *
 * Arguments will be registered as arrays of argument specs that
 * belong to a particular group of software modules. Each group will
 * be allowed to register a callback function so that the arguments,
 * when they are recognized, can trigger some appropriate behavior.
 *
 * The argument descriptions should also be complete enough that the
 * class could even produce some help text.
 *
 * The information to be recorded per argument group will be:
 *
 *   - GroupID, Optional CallBack Object Reference and CallBackup Method
 *     Name, Array of arguments associated with this groupID.
 *
 *   The information contained in an array of arguments includes the
 *   following items per argument:
 *
 *   - Argument ID: reference ID for this argument
 *
 *   - Array of arg texts to recognize, ie: -q and --quiet for 'quiet'
 *
 *   - Number of additional parameters that follow this arg, 
 *        ie: --config <name> requires 1 additional parameter.
 *
 *   - Optional array of default values (1 value per parameter.) These
 *     values are for documentation purposes and represent the
 *     starting value for the property after initializatoin of the
 *     application but before the corresponding argument is processed.
 *
 *   - Optional value to set for this argument. This information is
 *     needed when setting flags that do not take any parameters.
 *
 *   - Optional array of property names for this option - to be passed
 *     to the callback function when getting and setting the option.
 *
 *   - Short example text for this argument and its parameters.
 *
 *   - Medium description text for this argument and its parameters.
 *
 *   For convenience the code should accept individual values instead
 *   of arrays of values for cases where the number of parameters is
 *   zero or one.
 *
 *   Example:
 *
 *   $args = array( "config" => array( "args"    => array( "-c", "--config" ),
 *                                     "params"  => 1,
 *                                     "prop"    => array( "app_config" ),
 *                                     "default" => array( "/etc/application/app.conf" ),
 *                                     "test"    => array( "file-exists" ),
 *                                     "example" => "--config <file>",
 *                                     "desc"    => "Set the path and file name of the application configuration file.",
 *                                   ),
 *
 *                  "quiet"  => array( "args"    => array( "-q", "--quiet" ),
 *                                     "params"  => 0,
 *                                     "prop"    => "quiet_flag",
 *                                     "default" => false,
 *                                     "value"   => true,
 *                                     "example" => "--quiet",
 *                                     "desc"    => "Force the application to run without issuing any messages to the console.",
 *                                   ),
 *                );
 * 
 * Available Parameter Value tests for validity:
 *
 * - file-not-exists:  valid if the file does not exist
 * - file-exists:      valid if the file exists
 * - dir-not-exists:   valid if the folder does not exist
 * - dir-exists:       valid if the folder exists
 * - email:            valid if the string contains an @ followed by a .
 * - string:           always valid
 * - date:             valid if the PHP strtotime function can parse the value
 * - time:             valid if the PHP strtotime function can parse the value
 * - stamp:            valid if the PHP strtotime function can parse the value
 * - bool:             valid if the 
 * - truefalse:        valid if the value is either true or false
 * - yesno:            valid if the value is either yes or no
 * - onoff:            valid if the value is either on or off
 * - int:              valid if the integer value of the string is identical to the string
 * - float:            valid if the float value of the string is identical to the string
 * 
 *
 *
 * Callback handlers:
 *
 * Objects that need to be notified when a command line arg has been
 * scanned can set a callback handler to receive the notification. The
 * handler will look like this:
 *
 *    function callback( $group_id,
 *                       $arg_id, $param_count, $args,
 *                       $arg_spec
 *                     )
 *    {
 *       echo "at callback( {$group_id}, {$arg_id}, {$param_count}, args, arg_spec )\n";
 *       var_dump( $args );
 *    }
 * 
 * Where:
 *
 * - $group_id is the group id that registered the matched argspec
 *
 * - $arg_id is the id of the arg (from the argspec) that matches the
     scanned argument.
 *
 * - $param_count is the number of parameters after the arg, ie:
 *   "--conf path/file" has one parameter, the path/file.
 *
 * - $args is an array containing the items found, ie: $args = array(
 *   "--conf", "path/file" ) for a valid reference to a configuration
 *   file. Note that the first element of the array is always the
 *   argument that was matched and the subsequent elements contain the
 *   parameters to the argument.
 *
 * - $arg_spec is the argument specification that was registered by
     the module.
 *
 *
 * <p><b>TODO: Help</b>
 *
 * At this point there is enough information in the args array to make
 * it possible to display all possible command line options - but more
 * info is needed to produce a full help text. In particular a set of
 * bare args specs would be needed along with a help text file.
 *
 * Once all the information is registered with the class it should be
 * possible to execute a simply method call to produce a full help
 * screen that is automatically maintained whenever any of the loaded
 * software modules are updated.

 * <p><b>TODO: Bare Args</b>
 *
 * Most command line or system applications are designed to interpret
 * bare arguments in some way. One example would be a command oriented
 * app which determines the meaning of the bare args according to the
 * command option that was specified at the start of the command line.
 *
 * In any case the current software is able to list all the bare args
 * that it finds in the ::bare_args array - but it is not able to
 * validate the args or trigger a callback under any conditions to
 * initiate any processing. It would be nice if there was some way to
 * list all the possible combinations of bare args and command options
 * so that the class could simply determine which combination to use
 * and execute an appropriate callback by itself.
 *
 * <p><b>(done): option validation</b>
 *
 * It would be nice if each command line option could accept a "test"
 * parameter, ie: "test" => "<something>" where <something> might be
 * to test for a valid date/time/int/float/bool, etc. It would also be nice
 * to be able to test for files that exist or do not exist or folders
 * that exist or do not exist. This would probably simplify the coding
 * of a great many system applications in the future.
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



define( "GENERAL_ARGS_MAGIC_TOKEN", 123987 );
define( "GENERAL_ARGS_MAXPARAMS",        7 );


class general_arg_groups_c
{
   var $id;

   var $label;
   var $description;

   var $callback_obj;
   var $callback_method;


   function __construct( $id, $label, $desc )
   {
      $this->id              = $id;
      $this->label           = $label;
      $this->description     = $desc;

      $this->callback_obj    = NULL;
      $this->callback_method = NULL;
   }


   function set_callback( $id, &$callback_obj, $callback_method )
   {
      if ( $this->id != $id )
         throw new Exception( "Group ID '{$this->id}' != '{$id}'" );

      $this->callback_obj    = &$callback_obj;
      $this->callback_method = $callback_method;
   }


   function send_info( $group_id, $arg_id, $param_count, $arg_list, $arg_spec )
   {
      if ( !is_object( $this->callback_obj ) )
         return; // no callback has been set

      $method = $this->callback_method;
      $this->callback_obj->$method( $group_id, 
                                    $arg_id, $param_count, $arg_list, 
                                    $arg_spec );
   }
}



class general_args_c
{
   static $init_complete;
   static $groups;
   static $argspecs;
   static $maxparams;     // max # params per arg

   static $command;       // name of the current script
   static $args;          // all args after the script name

   static $valid_args;    // array listing same info as passed to send_info()
   static $bare_args;     // array listing bare args (unknown info found)
   static $scan_result;   // array listing valid and bare args in the order found
   static $errors;        // array listing invalid args (not found or incorrect param count)


   private static function _init()
   {
      if ( general_args_c::$init_complete === GENERAL_ARGS_MAGIC_TOKEN )
         return;

      general_args_c::$init_complete = GENERAL_ARGS_MAGIC_TOKEN;
      general_args_c::$groups        = array();
      general_args_c::$argspecs      = array();

      general_args_c::$command       = "";
      general_args_c::$args          = array();
      general_args_c::$valid_args    = array();
      general_args_c::$bare_args     = array();
      general_args_c::$scan_result   = array();
      general_args_c::$errors        = array();

      general_args_c::$maxparams     = GENERAL_ARGS_MAXPARAMS;
   }


   function __construct()
   {
      general_args_c::_init();
   }


  /**
   *
   * Reset() the various results arrays.
   *
   * The reset method will clear all the results from previous scans
   * of the command line args. In most cases this will not be
   * necessary - so far this method has been used for testing only.
   *
   */
   static function reset()
   {
      general_args_c::$command       = "";
      general_args_c::$args          = array();
      general_args_c::$valid_args    = array();
      general_args_c::$bare_args     = array();
      general_args_c::$errors        = array();
   }


   function get_errors()
   {
      return general_args_c::$errors;
   }


   function get_bare_args()
   {
      return general_args_c::$bare_args;
   }


  /**
   *
   * get_arg() will return one argument (after scanning the command
   * line) or it can get all the arguments related to a particular
   * group of arguments.
   *
   * get_arg() takes one or two parameters: the first one is the
   * $group_id. The second argument is optional: The $arg_id. If not
   * provided get_arg() will return all the args that were scanned
   * from the command line for the specified group. If provided with
   * an $arg_id get_arg() will return only that one argument.
   *
   * The information is retrieved from the $valid_args[] array. This
   * array is only valid after the scan() method has been called to
   * scan the command line.
   *
   * @param string $group_id the ID of the argument group to search.
   * @param string $arg_id the ID of a specific argument.
   * @retval array The array( $group_id, $arg_id, $params,
   *         $args_found, $arg_spec ) information that was saved when
   *         the scanner matched the argument on the command line with
   *         the registered argspec for the group_id. Args found is an
   *         array containing all the items in an argument. For
   *         example: "--conf <file>" would return $args_found =
   *         array( "--conf", "<file>" ); The Arg Spec element
   *         contains a copy of the entire argument specification that 
   *         was registered for the argument.
   *
   */
   static function get_arg( $group_id, $arg_id = false )
   {
      if ( $arg_id === false )
      {
         if ( !isset( general_args_c::$valid_args[$group_id] ) )
            return false;

         return general_args_c::$valid_args[$group_id];
      }

      if ( !isset( general_args_c::$valid_args[$group_id][$arg_id] ) )
         return false;

      return general_args_c::$valid_args[$group_id][$arg_id];
   }



  /**
   *
   * set_callback() handler for objects that want to be notified when a valid command line arg has been successfully scanned.
   *
   * The notifications are sent in the same format as the send_notify() method below.
   *
   */
   static function set_callback( $id, &$callback_obj, $callback_method )
   {
      general_args_c::_init();
      general_args_c::$groups[$id]->set_callback( $id, $callback_obj, $callback_method );
   }


  /**
   *
   * Register() a group of argspecs with this class.
   *
   * The register() method is used by various modules to register
   * groups of argspecs that will be recognized by this class when the
   * application command line is scanned. Each module can also set a
   * callback handler so that it can be notified when an arg has been
   * successfully scanned.
   *
   */
   static function register( $id, $label, $description, &$argspecs )
   {
      general_args_c::_init();

      general_args_c::$argspecs[$id] = &$argspecs;
      general_args_c::$groups[$id]   = new general_arg_groups_c( $id, $label, $description );
   }


  /**
   *
   * Show() the arguments that the application has registered.
   *
   * The show() method loops through the various argument groups and
   * displays them to the console. Note that output is generated using
   * echo commands and is not prefixed or wrapped in any way. If the
   * output must be redirected to stderr or otherwise massaged it is
   * necessary to capture it.
   *
   */
   static function show( $id = false )
   {
      general_args_c::_init();

      if ( $id === false )
      {
         $id_list = array_keys( general_args_c::$groups );
         foreach( $id_list as $id )
            general_args_c::show( $id );

         return;
      }

      $label   = general_args_c::$groups[$id]->label;
      $desc    = general_args_c::$groups[$id]->description;

      echo "{$label} - {$desc}\n\n";

      $arglist = general_args_c::$argspecs[$id];

      foreach ( $arglist as $a => $spec )
      {
         $args    = $spec["args"];
         $params  = $spec["params"];
         $default = isset( $spec["default"] ) 
                       ? $spec["default"]
                       : NULL;
         $ex      = $spec["example"];
         $desc    = $spec["desc"];

         if ( is_array( $args ) )
         {
            foreach ( $args as $as )
               echo "  {$as}\n";
         }
           else echo "  {$args}\n";

         echo "    Example: \"{$ex}\"\n";
         echo "    {$desc}\n";

         echo "\n";
      }
   }


   private static function send_info( $group_id, 
                                      $arg_id, $param_count, $arg_list, 
                                      $arg_spec )
   {
      if ( !isset( general_args_c::$groups[$group_id] ) )
         throw new Exception( "unknown argument group id {$group_id}" );

      general_args_c::$groups[$group_id]->send_info( $group_id, 
                                                     $arg_id, $param_count, $arg_list, 
                                                     $arg_spec );
   }


   static function adjust_arg_value( $val, $test )
   {
      switch ( $test )
      {
         case "file-not-exists":
         case "file-exists":
         case "dir-not-exists":
         case "dir-exists":
         case "email":
         case "string":
            return $val;

         case "date":
         case "time":
         case "stamp":
            return strtotime( $val );

         case "bool":
         case "truefalse":
            return strtolower($val) == "true" ? true : false;

         case "yesno":
            return strtolower($val) == "yes" ? true : false;

         case "onoff":
            return strtolower($val) == "on" ? true : false;

         case "int":
            return (int)$val;

         case "float":
            return (float)$val;

         default:
            throw new Exception( "unknown test: {$test}" );
      }
   }


   static function _test_arg_valid( $val, $test )
   {
      switch ( $test )
      {
         case "file-not-exists":
            return file_exists( $val ) ? "file '{$val}' exists" : "";

         case "file-exists":
            return file_exists( $val ) ? "" : "file '{$val}' does not exist";

         case "dir-not-exists":
            return file_exists( $val ) ? "folder '{$val}' exists" : "";

         case "dir-exists":
            return file_exists( $val ) ? "" : "folder '{$val}' does not exist";

         case "date":
         case "time":
         case "stamp":
            return strtotime( $val ) === false ? "date/time '{$val}' is not valid" : "";

         case "string":
            return "";

         case "bool":
         case "truefalse":
            $val = strtolower($val);
            return $val == "true" || $val == "false" ? "" : "expecting true/false, found '{$val}'";

         case "yesno":
            $val = strtolower($val);
            return $val == "yes" || $val == "no" ? "" : "expecting yes/no, found '{$val}'";
         case "onoff":
            $val = strtolower($val);
            return $val == "on" || $val == "off" ? "" : "expecting on/off, found '{$val}'";

         case "int":
            return ((int)$val) == $val ? "" : "expecting an integer, found '{$val}'";

         case "float":
            return ((float)$val) == $val ? "" : "expecting a number, found '{$val}'";

         case "email":
            if ( ($p1 = strpos( $val, "@" )) !== false )
               if ( ($p2 = strpos( $val, ".", $p1 )) !== false )
                  return "";

            return "'{$val}' does not appear to be an email address";

         default:
            throw new Exception( "unknown test: {$test}" );
      }
   }


   static function test_arg_valid(  $group_id, $arg_id, $param_count, &$args_found, &$arg_spec )
   {
      $error_msg = "";
      for ( $i = 0; $i < $param_count; $i++ )
         if ( ($err = general_args_c::_test_arg_valid( $args_found[$i+1], $arg_spec["test"][$i] )) > "" )
            $error_msg = $error_msg
                       . ($error_msg > "" ? "\n" : "")
                       . $err
                       ;
         else $args_found[$i+1] = general_args_c::adjust_arg_value( $args_found[$i+1], $arg_spec["test"][$i] );

      if ( $error_msg > "" )
      {
         general_args_c::$errors[] = array( $args_found[0], $error_msg );
         return false;
      }

      return true;
   }


   private static function _scan( &$args_found, &$parameter_count, $args_remaining, $arg )
   {
      $id_list    = array_keys( general_args_c::$groups );

      foreach( $id_list as $group_id )
      {
         $label   = general_args_c::$groups[$group_id]->label;
         $desc    = general_args_c::$groups[$group_id]->description;

         $arglist = general_args_c::$argspecs[$group_id];

         foreach ( $arglist as $arg_id => $arg_spec )
         {
            $args    = $arg_spec["args"];
            $params  = $arg_spec["params"];
            $default = isset( $arg_spec["default"] ) 
                          ? $arg_spec["default"]
                          : NULL;

            $ex      = $arg_spec["example"];
            $desc    = $arg_spec["desc"];

            if ( !is_array( $args ) )
              $args = array( $args );

            foreach ( $args as $a )
               if ( $arg[0] == $a )
               {
                  if ( $params > $args_remaining )
                  {
                     general_args_c::$errors[] = array( $a, "requires {$params} parameters, {$args_remaining} found" );
                     return false;
                  }

                  $parameter_count = $params;
                  for ( $j=0; $j < ($params+1); $j++ )
                     $args_found[$j] = $arg[$j];

                  $arg_is_valid = true;
                  if ( isset( $arg_spec["test"] ) )
                     if ( ($arg_is_valid = general_args_c::test_arg_valid( $group_id, $arg_id, $params, $args_found, $arg_spec )) != true )
                        return false;

                  general_args_c::$valid_args[$group_id][$arg_id] = array( $group_id, $arg_id, $params, $args_found, $arg_spec );
                  general_args_c::$scan_result[] = array( $group_id, $arg_id, $params, $args_found, $arg_spec );
                  general_args_c::send_info( $group_id, $arg_id, $params, $args_found, $arg_spec );
                  return true;
               }
         }
      }

      if ( substr( $arg[0], 0, 1 ) == "-" )
         general_args_c::$errors[] = array( $arg[0], "unknown option" );
      else 
      {
         general_args_c::$bare_args[]   = $arg[0];
         general_args_c::$scan_result[] = $arg[0];
      }

      return false;
   }


  /**
   *
   * Scan() the command line arguments.
   *
   * This method will scan the application command line arguments (or,
   * for testing purposes, the arguments passed via $argv,) and put
   * the results into various static arrays associated with this
   * class. The return result of this method is always a simply array
   * of results from the scan. Each element in the result array is a
   * command line argument. If the argument is a recognized option
   * with multiple parameters then those elements will be grouped into
   * an array of arg information (the same information passed to the
   * send_notify() method.) Otherwise, for bare args, the arg will
   * appear by itself.
   *
   * The internal arrays are:
   *
   * scan_result[] - An array containing both bare args (a single
   *                 string) and matched/scanned args (an array of arg
   *                 info) in the order that they were found.
   *
   * bare_args[]   - An array of bare args (single strings) in the order
   *                 they were found.
   *
   * valid_args[]  - An array of matched/scanned args (each one an
   *                 array of arg-related info,) in the order they
   *                 were found. Bare args are not included in this
   *                 array.
   *
   * errors[]      - An array of unknown arguments (each one a simple
   *                 string.) These are generally command line items
   *                 that are prefixed with a dash but are otherwise
   *                 unrecognized.
   *
   */
   static function scan( $argv = false )
   {
      general_args_c::_init();

      if ( $argv === false )
         $argv = $GLOBALS["argv"];

      general_args_c::$command = array_shift($argv);
      general_args_c::$args    = $argv;

      $result     = array();
      $total_args = count($argv);
      for ( $i = 0; $i < $total_args; $i++ )
      {
         $arg = array();
         for ( $j = 0; $j < general_args_c::$maxparams; $j++ )
            $arg[$j] = isset( $argv[$i+$j] ) ? $argv[$i+$j] : NULL;

         $args_remaining = $total_args - $i -1;

         $parameter_count = 0;
         $args_found      = array();
         if ( ($r = general_args_c::_scan( $args_found, $parameter_count, $args_remaining, $arg )) === true )
         {
            $i       += $parameter_count;
            $result[] = $args_found;
         }
           else 
         {
            $result[] = array( $arg[0] );
         }
      }

      return count(general_args_c::$errors) ? false : $result;
   }


   static function display_errors()
   {
      if ( count( general_args_c::$errors ) )
      {
         echo "Command line option errors:\n";
         foreach ( general_args_c::$errors as $err )
            echo "{$err[0]}: {$err[1]}\n";

         return true;
      }

      return false;
   }
}



