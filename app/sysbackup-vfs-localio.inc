<?php // -*- mode: php; mode: mmm; coding: utf-8-unix; -*-
/** 
 *
 * Virtual Backup Filesystem plugin - local filesystem
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



class backupvfs_localio_c extends backupvfs_baseio_c
{
   var $tar;

   function __construct( &$application_object, &$tar_object, $options = false )
   {
      parent::__construct( $application_object );

      $this->tar                = &$tar_object;

      $this->plugin_enabled     = false;
      $this->plugin_type        = "vfslocalio";
      $this->plugin_unique      = true;

      $this->plugin_name        = "vfs-localio";
      $this->plugin_title       = "Local Fileio VFS Plugin";
      $this->plugin_version     = "0.01a";
      $this->plugin_description = "Plugin to write the backupset to the local filesystem";
   }


   /**
    *
    * Plugin Instance
    *
    * return a string of the form:
    *
    * SSH remote VFS (user@server-name.tld)
    *
    */
   function plugin_instance()
   {
      return "Local VFS on " . $this->get_hostid();
   }



   function vfs_prune_expired_backups()
   {
      $description    = $this->get_hostname() . " daily";
      $path_to_prune  = $this->get_path_daily();
      $max_items      = $this->get_rotations_daily();

      return $this->_vfs_prune_expired_backups( $description, $path_to_prune, $max_items );
   }



   function vfs_addfile( $target, $source )
   {
      return $this->backupset->addfile( $target, $source );
   }



   function vfs_compress_string( $target, $source )
   {
      $tmp_file = tempnam( $this->path_temp, "sysbackup-" );

      $this->tar->last_exec_result(true); // reset the exec result
      if ( $this->tar->str( $tmp_file, $source ) != false )
         $this->_vfs_fatal_file_error( "unable to compress string", false, $tmp_file );

      return $this->backupset->addfile( $target, $tmp_file );
   }


   function vfs_compress_command( $cmd, $target )
   {
      $tmp_file = tempnam( $this->path_temp, "sysbackup-" );

      $this->tar->last_exec_result(true); // reset the exec result
      if ( $this->tar->command( $cmd, $tmp_file ) != false )
         $this->_vfs_fatal_file_error( "unable to get+compress command output", false, $cmd );

      return $this->backupset->addfile( $target, $tmp_file );
   }


   function vfs_compress_file( $target, $source )
   {
      $tmp_file = tempnam( $this->path_temp, "sysbackup-" );

      $this->tar->last_exec_result(true); // reset the exec result
      if ( $this->tar->file( $tmp_file, $source ) != false )
         $this->_vfs_fatal_file_error( "unable to compress file", $source, $tmp_file );

      return $this->backupset->addfile( $target, $tmp_file );
   }


   function vfs_tarball_folder( $target, $source, $options = array() )
   {
      $tmp_file = tempnam( $this->path_temp, "sysbackup-" );

      $this->tar->last_exec_result(true); // reset the exec result
      if ( $this->tar->folder( $tmp_file, $source, $options ) != false )
      {
         $msg = $this->tar->last_error();
         if ( strpos( $msg, SYS_TAR_FILECHANGED_TEXT ) )
         {
            $this->app->warning( "tarball may contain errors: {$target}" );
            $this->app->warning( "error message was: {$msg}" );
         }
           else $this->_vfs_fatal_file_error( "unable to tarball folder", $source, $tmp_file );
      }

      $msg = $this->tar->last_error();
      if ( strpos( $msg, SYS_TAR_FILECHANGED_TEXT ) !== false )
      {
         $t = str_replace( $this->tar->tarball_ext(), ".err", $target );
         $rel_ref = $t; // $target was redefined as a rel_ref a while ago
         $abs_ref = dir_fn( $this->path_backupset . "/" . $rel_ref );

         $msg = "tarball may contain errors; error message was:\n\n"
              . $msg
              ;

         if ( file_put_contents( $abs_ref, $msg ) === false )
            $this->fatal( "unable to write error message file: {$rel_ref}" );

         if ( chmod( $abs_ref, $this->get_umaskfiles() ) === false )
            $this->fatal( "unable to change mode on file: {$rel_ref}" );
      }

      return $this->backupset->addfile( $target, $tmp_file );
   }
}



