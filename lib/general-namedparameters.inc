<?php /* -*- mode: php; mode: mmm; coding: utf-8-unix; -*- */
/**
 *
 * General Named Function Parameters
 *
 * This is a feeble attempt at managing tree structures of named
 * function parameters (optional and various types of required
 * parameters.) It is the second major attempt, over the past decade,
 * to manage this issue. Unfortunately the results to-date have not
 * been as desired. In any case this code is still being used because
 * it is needed. Far too many class methods have too many parameters
 * that are hard to remember - so the programmers who use them need
 * error checking and useful error messages.
 *
 * Each List of Parameters has Category, Class, Method and Index
 * names. For example you can create a category for your company
 * customer database middleware: "ACME::CUST". The actual string is
 * not important but it must be unique.
 *
 * Within each category the parameters are grouped by Class. So if you
 * have a class called customer_io_base_c you might want to use
 * "customer_io" as the index for parameters related to the methods
 * provided by that base class.
 *
 * Within each class the parameters are grouped by a Method name. In
 * most cases this will be a method name such as write_customer.
 *
 * The real problem is that the customer_io_base_c will be extended
 * and the extended version of write_customer will require additional
 * parameters not needed by the method in the base class. Therefore
 * the parameter lists are broken down one last time by Index. The
 * index need not be numeric - it can be strings like BASE and
 * EXTENDED.
 *
 * The idea is that it is possible to aggregate all the parameter
 * lists within a given Category, Class & Method. The programmer can
 * then get a warning if an extra parameter is found in a named
 * parameter list (usually a typo in the code,) or if a required
 * parameter is missing (usually a programming error.)
 *
 * Because the parameter lists are stored in a static array they are
 * available to any method in any class. This allows helpful error
 * messages to be provided to programmers in cases where a method is
 * passing parameter lists to otherwise unrelated methods in another
 * class.
 *
 * The resulting parameter lists look like this:
 *
 * $named_parameters->all[Category][Class][Method][Index] = 
 *                        array( ParamName => array( <req-flag>, [def-val,]
 *                                                   [local-name,]
 *                                                   <help-text> ),
 *                               ParamName => array( <req-flag>, [def-val,]
 *                                                    [local-name,]
 *                                                    <help-text> ), ... ]
 *                             );
 *
 *
 * The required flag and help-text are required. The default value and
 * local variable name are optional. Note that you must provide a
 * default value if you want to define a local variable name. These
 * features simplify the unloading of named parameters into local
 * method variables.
 *
 * Because the parameter lists can include default values and help
 * strings it is possible to loop through the list to produce helpful
 * messages for the programmer.
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


/**
 * Requirement setting for parameters
 *
 * Each parameter must have one of the following requirement settings:
 *
 */
define( "NAMEDPARAM_INTERNAL",     -1 ); // Required, can be or is provided internally
define( "NAMEDPARAM_OPTIONAL",  false ); // Optional
define( "NAMEDPARAM_REQUIRED",  true  ); // Required
define( "NAMEDPARAM_DEFAULT",       2 ); // Required but default value available


/**
 * Requirement setting (short form)
 */
define( "NPINT", NAMEDPARAM_INTERNAL );
define( "NPOPT", NAMEDPARAM_OPTIONAL );
define( "NPREQ", NAMEDPARAM_REQUIRED );
define( "NPDEF", NAMEDPARAM_DEFAULT  );


require_once "general-tee.inc";


class named_parameters_base_c 
{
   /**
    * global array of all required parameter lists
    *
    * $all["category"]["class"]["method"]["index"] = 
    *
    *     array( "param" => array( <req>, [<default>,] [<var-name>,] <help> ) 
    *            [, ...]
    *          );
    *
    */
   static $all;    // globally accessible array of names parameters
   static $teelog; // reference to the logging/display class used by the application


   function __construct()
   {
   }


   function set_logger( &$teelog )
   {
      named_parameters_c::$teelog = $teelog;
   }


   function tee( $msg, $eol = "\n" )
   {
      if ( is_object( named_parameters_c::$teelog ) )
      {
         named_parameters_c::$teelog->tee( $msg, $eol );
         return;
      }

      // emit a message
      echo "{$msg}{$eol}";
   }


   function fatal( $msg, $stop_flag = true )
   {
      if ( is_object( named_parameters_c::$teelog ) )
         named_parameters_c::$teelog->fatal( $msg, $stop_flag );

      if ( $stop_flag )
         throw new Exception( TEELOG_TXTFATAL . $msg );
      else $this->tee( TEELOG_TXTFATAL . $msg );
   }


   function warning( $msg )
   {
      if ( is_object( named_parameters_c::$teelog ) )
      {
         named_parameters_c::$teelog->warning( $msg );
         return;
      }

      $this->tee( TEELOG_TXTWARNING . $msg );
   }


   function reqstr( $r )
   {
      switch ( $r )
      {
         case NPINT:   // required information, provided via other values
            return "int";

         case NPDEF:   // required information, default values available
            return "def";

         case NPREQ:   // required information
         case true:
            return "req";

         case NPOPT:   // optional information - not necessarily needed
         case false:
            return "opt";

         default:
            $this->fatal( "unknown required option code" );
      }
   }


   function _register( $cat, $cls, $meth, $idx, &$opt_list )
   {
      $options = array();
      foreach( $opt_list as $opt => $info )
      {
         if ( is_string( $info ) )
            $info = array( NPREQ, $info );

         if ( !is_array( $info ) )
         {
            $this->fatal( "invalid parameter info passed for {$opt}" );
         }

         $reqflag = $info[0];
         $defval  = NULL;
         $help = NULL;
         $varname = $opt;

         switch ( count($info) )
         {
            case 4:
               $defval  = $info[1];

               // ignore false value (placeholder) in this field
               if ( $info[2] !== false )
                  $varname = $info[2];

               $help    = $info[3];
               break;

            case 3:
               $defval  = $info[1];
               $help    = $info[2];
               break;

            case 2:
               $help    = $info[1];
               break;

            default:
               $this->fatal( "invalid number of items supplied for {$opt}" );
               return true; // can't get here - but return error anyway.
         }

         named_parameters_c::$all[$cat][$cls][$meth][$idx][$opt] = 
                                 array( $reqflag, $defval, $varname, $help );
      }
   }


   function _get_params( $cat, $cls, $meth, $idx )
   {
      if ( $idx === false )
      {
         if ( !isset( named_parameters_c::$all[$cat][$cls][$meth] ) )
            $this->fatal( "Unknown parameter set( {$cat}, {$cls}, {$meth} )" );

         $opt_list = named_parameters_c::$all[$cat][$cls][$meth];

         $params = array();
         foreach ( $opt_list as $idx => $param_list )
            $params = array_merge( $params, $param_list );

         return $params;
      }

      if ( !isset( named_parameters_c::$all[$cat][$cls][$meth][$idx] ) )
         $this->fatal( "Unknown parameter set[{$cat}][{$cls}][{$meth}][{$idx}] ?" );

      return named_parameters_c::$all[$cat][$cls][$meth][$idx];
   }

   function _get_param( $cat, $cls, $meth, $idx, $opt )
   {
      return named_parameters_c::$all[$cat][$cls][$meth][$idx][$opt];
   }

   function _get_required( $cat, $cls, $meth, $idx, $opt )
   {
      return named_parameters_c::$all[$cat][$cls][$meth][$idx][$opt][0];
   }

   function _get_default( $cat, $cls, $meth, $idx, $opt )
   {
      return named_parameters_c::$all[$cat][$cls][$meth][$idx][$opt][1];
   }

   function _get_varname( $cat, $cls, $meth, $idx, $opt )
   {
      return named_parameters_c::$all[$cat][$cls][$meth][$idx][$opt][2];
   }

   function _get_help( $cat, $cls, $meth, $idx, $opt )
   {
      return named_parameters_c::$all[$cat][$cls][$meth][$idx][$opt][3];
   }

   function _get_showhelp_flag( &$show_help_flag )
   {
      if ( $show_help_flag !== -1 )
         return $show_help_flag;

      if ( is_object( named_parameters_c::$teelog ) )
         $show_help_flag = named_parameters_c::$teelog->debug_params_showhelp;

      return $show_help_flag;
   }


   function _dump_val( $val, $name, $pinf, $show_help_flag = -1 )
   {
      $val = phpval2str( $val );
      $this->_get_showhelp_flag( $show_help_flag );

      $required_flag = $this->reqstr( $pinf[0] );

      if ( $show_help_flag )
      {
         $help_string   = $pinf[3];

         $property_name = $pinf[2] ? ("aka '{$pinf[2]}'"         
                                     . ($help_string ? ", " : "")) 
                                   : "";

         $default_value = $pinf[1] ? ("defaults to " . phpval2str($pinf[1]) 
                                     . ($help_string ? ", " : ""))
                                   : "";

         $this->tee( sprintf( "  %3s %-24s %-55s %s%s%s", $required_flag, 
                                                          $name, $val, 
                                                          $default_value, $property_name, $help_string ));
      }
        else 
      {
         $default_value = $pinf[1] ? "defaults to " . phpval2str($pinf[1]) 
                                   : "";

         $this->tee( sprintf( "  %3s %-24s %-55s %s", $required_flag, 
                                                      $name, $val, 
                                                      $default_value ));
      }
   }


   function _display_val( $name, $val, $pinf, $show_help_flag = -1 )
   {
      $val = phpval2str( $val );
      $this->_get_showhelp_flag( $show_help_flag );

      if ( $show_help_flag )
         $this->tee( sprintf( "  %3s %-24s %-55s %s", 
                              $this->reqstr($pinf[0]), $name, $val, $pinf[3] ));
      else $this->tee( sprintf( "  %3s %-24s %s", 
                                $this->reqstr($pinf[0]), $name, $val ));
   }


   function __dump_properties( &$obj, $required_options, $show_help_flag = -1 )
   {
      foreach ( $required_options as $name => $pinfo )
      {
         $propname = $pinfo[2] === false ? $name : $pinfo[2];
         $this->_dump_val( $obj->$propname, $name, $pinfo, $show_help_flag );
      }
   }


   function _dump_properties( &$obj, $cat, $cls, $meth, $idx, $show_help_flag = -1 )
   {
      $required_options = $this->_get_params( $cat, $cls, $meth, $idx );
      $this->__dump_properties( &$obj, $required_options, $show_help_flag );
   }


   function _dump_paramvals( &$params, $cat, $cls, $meth, $idx, $show_help_flag = -1 )
   {
      $required_options = $this->_get_params( $cat, $cls, $meth, $idx );

      foreach ( $required_options as $name => $pinfo )
         if ( isset( $params[$name] ) )
            $this->_dump_val( $name, $params[$name], $pinfo, $show_help_flag );
   }


   function _display_required( $cat, $cls, $meth, $idx, $show_help_flag = -1 )
   {
      $required_options = $this->_get_params( $cat, $cls, $meth, $idx );

      foreach ( $required_options as $name => $pinfo )
         $this->_display_val( $name, "-", $pinfo, $show_help_flag );
   }


   function _display_properties( &$obj, $cat, $cls, $meth, $idx, $show_help_flag = -1 )
   {
      $required_options = $this->_get_params( $cat, $cls, $meth, $idx );

      foreach ( $required_options as $name => $pinfo )
      {
         $varname = $pinfo[2];
         $this->_display_val( $name, $obj->$varname, $pinfo, $show_help_flag );
      }
   }


   function _display_paramvals( &$params, $cat, $cls, $meth, $idx, $show_help_flag = -1 )
   {
      $required_options = $this->_get_params( $cat, $cls, $meth, $idx );

      foreach ( $required_options as $name => $pinfo )
         if ( isset( $params[$name] ) )
            $this->_display_val( $name, $params[$name], $pinfo, $show_help_flag );
   }


   function _check( $cat, $cls, $meth, $idx, &$opt_list )
   {
      $required_options = $this->_get_params( $cat, $cls, $meth, $idx );

      $option_errors     = false;
      foreach ( $required_options as $opt => $params )
      {
         $req_flag = $params[0];
         $help_txt = $params[3];

         if ( $req_flag && !isset($opt_list[$opt]) )
         {
            $option_errors = true;
            $this->warning( "Missing \"{$opt}\" parameter ({$help_txt})" );
         }
      }

      if ( $idx === false )
         foreach( $opt_list as $opt => $params )
            if ( !isset( $required_options[$opt] ) )
            {
               $option_errors = true;
               $this->warning( "Parameter \"{$opt}\" is not recognized" );
            }

      if ( $option_errors )
         foreach ( $required_options as $opt => $params )
         {
            $req_flag = $params[0];
            $help_txt = $params[3];

            if ( !$req_flag && !isset($opt_list[$opt]) )
               $this->warning( "Check also the \"{$opt}\" parameter ({$help_txt})" );
         }

      if ( $option_errors )
         $this->fatal( "Missing or invalid parameters" );
   }


   function _set_properties( &$obj, $cat, $cls, $meth, $idx, &$opt_list )
   {
      $required_options = $this->_get_params( $cat, $cls, $meth, $idx );

      foreach ( $required_options as $opt => $params )
      {
         $varname = $params[2];
         $obj->$varname = $opt_list[$opt];
      }
   }


   function _get_valid( $cat, $cls, $meth, $idx, &$opt_list )
   {
      $required_options = $this->_get_params( $cat, $cls, $meth, $idx );

      $valid_params = array();
      foreach ( $required_options as $opt => $params )
         if ( isset( $opt_list[$opt] ) )
            $valid_params[$opt] = $opt_list[$opt];

      return $valid_params;
   }


   function _set_optional( $cat, $cls, $meth, $idx, &$opt_list )
   {
      $required_options = $this->_get_params( $cat, $cls, $meth, $idx );

      foreach ( $required_options as $opt => $params )
         if ( !isset( $opt_list[$opt] ) )
            $opt_list[$opt] = $required_options[$opt][1];
   }
}



class named_parameters_c extends named_parameters_base_c
{
   var $current_category;
   var $current_class;

   function __construct( $cat, $cls )
   {
      $this->current_category = $cat;
      $this->current_class    = $cls;
   }


   function register( $meth, $idx, $opt_list )
   {
      $this->_register( $this->current_category,
                        $this->current_class,
                        $meth, $idx, $opt_list );
   }


   function get_params( $meth, $idx )
   {
      return $this->_get_params( $this->current_category,
                                 $this->current_class,
                                 $meth, $idx );
   }


   function get_param( $meth, $idx, $opt )
   {
      return $this->_get_param( $this->current_category,
                               $this->current_class,
                               $meth, $idx, $opt );
   }


   function get_required( $meth, $idx, $opt )
   {
      return $this->_get_required( $this->current_category,
                                   $this->current_class,
                                   $meth, $idx, $opt );
   }


   function get_default( $meth, $idx, $opt )
   {
      return $this->_get_default( $this->current_category,
                                  $this->current_class,
                                  $meth, $idx, $opt );
   }


   function get_varname( $meth, $idx, $opt )
   {
      return $this->_get_varname( $this->current_category,
                                  $this->current_class,
                                  $meth, $idx, $opt );
   }


   function get_help( $meth, $idx, $opt )
   {
      return $this->_get_help( $this->current_category,
                               $this->current_class,
                               $meth, $idx, $opt );
   }


   function dump_properties( &$obj, $meth, $idx, $show_help_flag = -1 )
   {
      $this->_dump_properties( $obj,
                            $this->current_category,
                            $this->current_class,
                            $meth, $idx, $show_help_flag );
   }


   function dump_paramvals( &$params, 
                             $meth, $idx, $show_help_flag = -1 )
   {
      $this->_dump_paramvals( $params,
                              $this->current_category,
                              $this->current_class,
                              $meth, $idx, $show_help_flag );
   }


   function display_required( $meth, $idx, $show_help_flag = -1 )
   {
      $this->_display_required( $this->current_category,
                                $this->current_class,
                                $meth, $idx, $show_help_flag );
   }


   function display_paramvals( &$params, 
                                $meth, $idx, $show_help_flag = -1 )
   {
      $this->_display_paramvals( $params,
                                 $this->current_category,
                                 $this->current_class,
                                 $meth, $idx, $show_help_flag );
   }


   function check( $meth, $idx, &$opt_list )
   {
      $this->_check( $this->current_category,
                     $this->current_class,
                     $meth, $idx, $opt_list );
   }


   function display_properties( &$obj, $meth, $idx, $show_help_flag = -1 )
   {
      $this->_display_properties( $obj,
                               $this->current_category,
                               $this->current_class,
                               $meth, $idx, $show_help_flag );
   }


   function set_properties( &$obj, $meth, $idx, &$opt_list )
   {
      $this->_set_properties( $obj, 
                              $this->current_category,
                              $this->current_class,
                              $meth, $idx, $opt_list );
   }


   function get_valid( $meth, $idx, &$opt_list )
   {
      return $this->_get_valid( $this->current_category,
                                $this->current_class,
                                $meth, $idx, $opt_list );
   }


   function set_optional( $meth, $idx, &$opt_list )
   {
      return $this->_set_optional( $this->current_category,
                                   $this->current_class,
                                   $meth, $idx, $opt_list );
   }
}



