<?php // -*- mode: php; mode: mmm; coding: utf-8-unix; -*-
/** 
 *
 * Base Plugin class for the SysBackup Plugins
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



/**
 *
 * Basic SysBackup plugin class
 *
 * All sysbackup plugins should extend this class
 *
 * @package sysbackup
 * @subpackage sysbackup
 *
 */
class sysbackup_plugin_base_c extends plugin_base_c
{
   private $tar;

   var     $subfolder;
   var     $job_info;



  /**
   *
   * verbose() returns true if the verbose flag is set
   *
   */
   function verbose()
   {
      return $this->app->verbose();
   }


  /**
   *
   * Encode Filenames
   *
   * It's important to encode tarball and compressed file backup file
   * names in a way that makes it easy to identify where the
   * associated files belong.
   *
   * Note that prefixes should not be encoded and should not use any
   * special characters.
   *
   */
   function encode_filename( $target )
   {
      $target = str_replace( "-", "--", $target );
      $target = str_replace( "/", "-",  $target );

      if ( substr( $target, 0, 1 ) == "-" )
        $target = substr( $target, 1 );

      return urlencode( $target );
   }



  /**
   *
   * rel_ref( $target )
   *
   * Return an encoded filename with a relative path from the base of the backupset.
   *
   * Target is a string that represents information about the object
   * being backed-up. It can include the original path to the original
   * file (including slash characters,) spaces and unicode
   * characters. The entire string is URL encoded with dash characters
   * encoded as double-dash and slash characters encoded as dash
   * characters. This is intended to help the sysadmin figure out
   * what's in a tarball or where it came from. For example a tarball
   * of the "/usr/share/doc" folder would be encoded as usr-share-doc.tgz.
   *
   * The currently selected subfolder in the backupset is then
   * prepended to the target string to form a relative path from the
   * base of the backupset. For example if the above tarball is being
   * written to the current subfolder "misc-info" in the backupset
   * this function would return misc-info/usr-share-doc.tgz as the
   * relative reference for the file.
   *
   */
   function rel_ref( $target )
   {
      return dir_fn( $this->subfolder, $this->encode_filename( $target ) );
   }


  /**
   *
   * Return the extension that the system is configured to use for
   * compressed files. For example if the system is configured to use
   * gzip to compress files the return result will be ".gz"
   *
   */
   function compressed_file_ext()
   {
      return $this->tar->compressor_ext();
   }


  /**
   *
   * same as above - for compatibility
   *
   * Return the extension that the system is configured to use for
   * compressed files. For example if the system is configured to use
   * gzip to compress files the return result will be ".gz"
   *
   */
   function compressor_ext()
   {
      return $this->tar->compressor_ext();
   }


  /**
   *
   * Return the extension that the system is configured to use for
   * tarballs. For example if the system is configured to use
   * tar and gzip the return result might be ".tgz" or ".tar.gz"
   *
   */
   function tarball_ext()
   {
      return $this->tar->tarball_ext();
   }



  /**
   *
   * vfs_addfile( $target, $source_abs_ref )
   *
   * Use this method to add the $source_abs_ref file to the virtual
   * file system during the backup.
   * 
   * NOTE: TARGET IS NOT A FILE NAME!! $target is a string that
   * represents the contents being added to the backupset. See the
   * notes for the rel_ref() method.
   *
   * See the sys-tarballs.inc module.
   *
   * Returns boolean false if there are no problems.
   *
   */
   function vfs_addfile( $target, $source_abs_ref )
   {
      return $this->app->vfs_addfile( $this->rel_ref( $target ), $source_abs_ref );
   }


  /**
   *
   * vfs_compress_string( $target, $string )
   *
   * Use this method to compress the contents of $string into a file
   * and add the file to the virtual file system during the backup.
   * 
   * NOTE: TARGET IS NOT A FILE NAME!! $target is a string that
   * represents the contents being added to the backupset. See the
   * notes for the rel_ref() method.
   *
   * See the sys-tarballs.inc module.
   *
   * Returns boolean false if there are no problems.
   *
   */
   function vfs_compress_string( $target, $source_str )
   {
     return $this->app->vfs_compress_string( $this->rel_ref($target), $source_str );
   }


  /**
   *
   * vfs_compress_command( $cmd, $target )
   *
   * Use this method to execute the provided command and compress the
   * resulting output into the file specified by $target.
   * 
   * NOTE: TARGET IS NOT A FILE NAME!! $target is a string that
   * represents the contents being added to the backupset. See the
   * notes for the rel_ref() method.
   *
   * See the sys-tarballs.inc module.
   *
   * Returns boolean false if there are no problems.
   *
   */
   function vfs_compress_command( $cmd, $target )
   {
      return $this->app->vfs_compress_command( $cmd, $this->rel_ref($target) );
   }


  /**
   *
   * vfs_compress_file( $target, $source_abs_ref )
   *
   * Use this method to compress the file specified in $source_abs_ref
   * and add the resulting file to the virtual file system during the
   * backup.
   * 
   * NOTE: TARGET IS NOT A FILE NAME!! $target is a string that
   * represents the contents being added to the backupset. See the
   * notes for the rel_ref() method.
   *
   * See the sys-tarballs.inc module.
   *
   * Returns boolean false if there are no problems.
   *
   */
   function vfs_compress_file( $target, $source_abs_ref )
   {
      return $this->app->vfs_compress_file( $this->rel_ref($target), $source_abs_ref );
   }


  /**
   *
   * vfs_tarball_folder( $target, $source_abs_ref, $options = array() )
   *
   * Use this method to tarball the folder specified in
   * $source_abs_ref and add the resulting file to the virtual file
   * system during the backup.
   * 
   * NOTE: TARGET IS NOT A FILE NAME!! $target is a string that
   * represents the contents being added to the backupset. See the
   * notes for the rel_ref() method.
   *
   * See the sys-tarballs.inc module.
   *
   * Returns boolean false if there are no problems.
   *
   */
   function vfs_tarball_folder( $target, $source_abs_ref, $options = array() )
   {
      return $this->app->vfs_tarball_folder( $this->rel_ref($target), 
                                             $source_abs_ref, $options );
   }



   /**
    *
    * Job Exec
    *
    * This method is called for EACH JOB during the event to which the
    * job was assigned. Jobs are sorted by Job Index string and
    * executed in sorted order.
    *
    * Your job_exec() method should call this parent method to take
    * care of basic setup (getting a copy of the job configuration
    * (job_info) and selecting the subfolder based on the setting in
    * the job configuration. The code here might get more complicated
    * in the future so it's a good idea to check for any error result
    * from this method and take appropriate action. Anything other
    * than boolean false is bad.
    *
    */
   function job_exec( $configuration )
   {
      $this->job_info = $configuration;

      $this->subfolder = "";
      if ( isset($this->job_info["subfolder"]) )
      {
         $this->subfolder = $this->job_info["subfolder"];
         if ( $this->app->vfs_subfolder( $this->subfolder ) != false )
            $this->app->fatal( "unable to select subfolder: {$this->subfolder}" );
      }
          
      // no problem detected
      return false;
   }


  /**
   *
   * a simple constructor.
   *
   */
   function __construct( &$application_object )
   {
      parent::__construct( $application_object );

      $this->tar = &$this->app->get_tar_object();

      // initially we don't know anything about a job to do
      // or a backupset subfolder to write tarballs into
      $this->job_info  = array();
      $this->subfolder = "";

     /**
      *
      * Copy the code below to your new plugin and correct the
      * assignments. See the Dummy plugin for documentation.
      *
      **************************************************************************/
      $this->plugin_name        = "";
      $this->plugin_title       = "";
      $this->plugin_version     = "";
      $this->plugin_description = "";

      $this->plugin_enabled     = false;  // true if the plugin is enabled
      $this->plugin_rootreqd    = false;  // true if root permissions required

      // most plugins are genbackup plugins
      $this->plugin_type        = "genbackup"; 

      // most plugins are not unique
      // they can be combined with others of their type
      $this->plugin_unique      = false;  
   }
}


