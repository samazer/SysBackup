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
 *    <ul>
 *
 *    <li>Determine the location of the temporary workspace
 *
 *    <li>Determine the name and location of the backup (the target
 *    folder for the backup being produced by the current program
 *    run.)
 *
 *    <li>Find the name and location of the previous successful
 *    backup, if any.
 *
 *    <li>Get the hashlist for the files that were written to the last
 *    successful backup (allowing the use of hardlinks to existing
 *    files in cases where those files are identical.)
 *
 *    </ul>
 *
 * <li>The class will provide a method for adding a sub-folder within
 * the backup set and selecting that sub-folder for use during a given
 * backup job.
 *
 * <li>The class will provide a method for compressing output piped
 * from a process and adding it to the backup set.
 *
 * <li>The class will provide a method for compressing a file on the
 * local filesystem and adding it to the backup set.
 *
 * <li>The class will provide a method for tarballing a folder on the
 * local filesystem and adding it to the backup set.
 *
 * <li>The class will list separately the tarballs that are hardlinked
 * so that future code can check for bit-rot.
 *
 * <li>The class will keep track of available disk space on the target
 * filesystem and on the local filesystem. The class will also keep
 * track of all items added to the backup set. If disk space on the
 * local filesystem is limited the class, when writing to a remote
 * filesystem, the class will copy the file immediately to allow the
 * local copy to be deleted. If a full backup is being saved to the
 * local filesystem the class will queue files for remote copy and
 * perform the copies after all other backup tasks have been
 * completed.
 *
 * <li>The class will implement a set of counters that will keep track
 * of disk space used, number of files written, number of files
 * hardlinked and any related information.
 *
 * <li>The class will provide a number of methods for use by the
 * backup rotation plugin (create folders, list folders and recursive
 * delete of folders.)
 *
 * </ul>
 *
 * Note that this backup software will often be running on a Virtual
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
 * As such the backup VFS must be able to operate on the local
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
 * @package server-maint
 * @subpackage sysbackup
 * @license http://www.gnu.org/licenses/agpl.html AGPL
 * @copyright 2011 by Sam Azer, All rights reserved
 * @link http://www.azertech.net/
 * @link http://www.samazer.net/about
 *
 */



require_once "lib/sys-tarballs.inc";
require_once "lib/sys-remote-fileio.inc";

require_once "app/sysbackup-vfs-base.inc";
require_once "app/sysbackup-vfs-localio.inc";
require_once "app/sysbackup-vfs-sshremote.inc";




class backupvfs_main_c extends plugin_framework_c
{
   var $backupset;        // name of the current backupset

   var $sys_timezone;     // default timezone to keep PHP happy
   var $sys_backupnamefmt;// format string for the date() function to generate backupset names
                          // embed \%\H\O\S\T\% to output %HOST% which is replaced with $sys_hostname

   var $sys_hostname;     // long host name used for naming of backup sets
   var $sys_keeplocal;    // true to keep a full copy of the backup on the local filesystem
   var $sys_pathtemp;     // path to directory to use for temp workspace
   var $sys_pathbackups;  // path to use for local storage of backups (if any)
   var $sys_pathrotations;//path to use for local storage of rotations (if any)
   var $sys_storedevice;  // device to query to determine available disk space

   var $sys_umaskfolders; // default umask to use for created directories
   var $sys_umaskfiles;   // default umask to use for created files

   var $sys_minbackupsize;// Minimum free space needed to hold one full backup (if desired)
   var $sys_minfreespace; // Minimum free space needed for operations. 
                          // This is usually the maximum size of the biggest 
                          // individual tarball in the backup set.

   var $tar;              // The object used to compress files and folders into tarballs

   var $sys_local;        // The local VFS object
   var $sys_remote;       // An array of remote VFS objects (id => $obj, id => obj, ... )
   var $sys_remotes;      // An array of remote VFS object names (id, id, ... )

   var $current_subfolder;// a string containing the name of the most recently selected sub-folder 
                          // in the backupset.

   var $max_job_subfolder_width; // max width of a subfolder name to help improve the appearance
                                 // of the "not-quiet" display output

   // array of files queued for transfer
   var $xfer_queue;       // queue[] = array( $rel_ref, $sha1, $size ), ...



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

      $this->named_parameters->register( $np_method, $np_index, array( 
         "sys_timezone"      => "the default timezone to use",
         "sys_backupnamefmt" => "format of the backup set name passed to the date() function (ie: \\%\\H\\O\\S\\T\\%\\-ymd)",
         "sys_hostname"      => "long host name used for naming of backup sets",
         "sys_keeplocal"     => "true to keep a full copy of the backup on the local filesystem",
         "sys_pathtemp"      => "path to directory to use for temp workspace",
         "sys_pathbackups"   => "path to use for local storage of backups (if any)",
         "sys_pathrotations" => "path to use for local storage of rotations (if any)",
         "sys_umaskfolders"  => "the umask to use for created directories (defaults to 0777)",
         "sys_umaskfiles"    => "the umask to use for created files (defaults to 0666)",
         "sys_storedev"      => "device to query to determine available disk space",
         "sys_minfreespace"  => "Minimum free space needed for operation.",
         "sys_minbackupsize" => "Minimum free space needed to store a full backup.",
                                                 ));

      $this->tar         = new mktarball_util_c();
      $this->sys_local   = new backupvfs_localio_c( $this, $opts );
      $this->sys_remote  = array();
      $this->sys_remotes = array();

      $this->sys_umaskfolders = 0777;
      $this->sys_umaskfiles   = 0666;
   }


   function setup_compression( $opts )
   {
      $this->tar->setup( $opts );
   }


   /**
    *
    * local setup can't be completed at construction time because the 
    * config file is not loaded until later.
    *
    */

   function setup_local()
   {
      $this->sys_local->hostname      = $this->sys_hostname;
      $this->sys_local->keeplocal     = $this->sys_keeplocal;
      $this->sys_local->path_temp     = $this->sys_pathtemp;
      $this->sys_local->path_backup   = $this->sys_pathbackups;
      $this->sys_local->path_rotation = $this->sys_pathrotations;
   }


   function add_remote( $id, &$obj )
   {
      if ( $obj->plugin_enabled() )
      {
         $this->sys_remotes[]   = $id;
         $this->sys_remote[$id] = $obj;
      } 
        else $this->tee( "Note: " . $obj->plugin_instance() . " is disabled" );
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
         $backupset_name = str_replace( "%HOST%", 
                                        $this->sys_hostname, 
                                        date($this->sys_backupnamefmt) );

      $this->backupset = $backupset_name;
      $this->sys_local->vfs_init($backupset_name);

      $this->teefn( dir_fn($this->sys_local->current_path, 
                           $this->sys_hostname . EXT_OUTPUTCAPTURE) );
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

      $tot = 0;
      $counter = "{$type}_tarball_count";
      foreach( $this->sys_remotes as $id )
         $tot += $this->sys_remote[$id]->$counter;

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


   function vfs_pathtemp()
   {
      return $this->sys_pathtemp;
   }


   function vfs_start()
   {
      if ( $this->verbose )
         $this->tee( "Local Virtual File System Initialized" );

      foreach( $this->sys_remotes as $id )
      {
         if ( $this->verbose )
            $this->tee( "Initializing Virtual File System '{$id}'" );

         $this->sys_remote[$id]->vfs_init($this->backupset);
      }

      $this->sys_local->vfs_start();
      foreach( $this->sys_remotes as $id )
         $this->sys_remote[$id]->vfs_start();
   }


   private function _remote_xferfile( $target, $hash, $size )
   {
      foreach( $this->sys_remotes as $id )
      {
         $start = microtime(true);
         if ( $this->notquiet() )
            $this->tee( "  {$id}:", "" );

         $hl  = $this->sys_remote[$id]->remote_canhardlink( $hash );
         $r   = $this->sys_remote[$id]->remote_copyfile( $target, $hash, $size );

         $tim = microtime(true) - $start;
         $dur = $this->duration($tim);

         if ( $this->notquiet() )
            $this->tee( ($hl ? "*" : "") . "{$dur}s" . ($r ? "??" : ""), "" );
      }
   }


   function remote_xferfiles()
   {
      if ( $this->not_quiet() )
         $this->summary_stats();

      $this->vfs_subfolder("");

      if ( $this->sys_keeplocal )
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
   }


   function vfs_end()
   {
      $this->sys_local->vfs_end();
      foreach( $this->sys_remotes as $id )
         $this->sys_remote[$id]->vfs_end();
   }


   function vfs_subfolder( $folder_name )
   {
      $this->current_subfolder = $folder_name;

      $this->sys_local->vfs_subfolder( $folder_name );
      foreach( $this->sys_remotes as $id )
         $this->sys_remote[$id]->vfs_subfolder( $folder_name );
   }


   function encode_filenames( $target )
   {
      $target = str_replace( "-", "--", $target );
      $target = str_replace( "/", "-",  $target );

      if ( substr( $target, 0, 1 ) == "-" )
        $target = substr( $target, 1 );

      return urlencode( $target );
   }


   private function _fileinfo( $filename, $subfolder, $info )
   {
      if ( $subfolder === false )
         $subfolder = $this->current_subfolder;

      $rel_ref = dir_fn( $subfolder, $filename );

      if ( !isset( $info[$rel_ref] ) )
         return false;

      return $info[$rel_ref];
   }


   function vfs_filehash( $filename, $subfolder = false )
   {
      return $this->_fileinfo( $filename, $subfolder, $this->sys_local->current_hashlist );
   }


   function vfs_filesize( $filename, $subfolder = false )
   {
      return $this->_fileinfo( $filename, $subfolder, $this->sys_local->current_sizelist );
   }


   function vfs_ishardlinked( $filename, $subfolder = false )
   {
      if ( !$this->sys_usehardlinks )
         return false;

      if ( $subfolder === false )
         $subfolder = $this->current_subfolder;

      $rel_ref = dir_fn( $subfolder, $filename );

      return isset( $this->sys_local->current_hardlinked[$rel_ref] );
   }


   function _vfs_local_cleanup( $target )
   {
      // are we keeping this file?
      if ( $this->sys_keeplocal )
         return false; // yes, return okay

      // no, delete it.
      $rel_ref = dir_fn( $this->app->current_subfolder . "/" . $target );
      $abs_ref = dir_fn( $this->current_path . "/" . $rel_ref );

      // sanity check
      if ( !file_exists( $abs_ref ) ) // unexpected condition (programming error?)
         $this->app->fatal( "expired file not found {$abs_ref}" );

      if ( unlink( $abs_ref ) )
         $this->app->fatal( "unable to remove expired file {$abs_ref}" );

      return false;
   }


   function _verbose_report( $target, $type = "file" )
   {
      $sub = $this->current_subfolder ? $this->current_subfolder : "/";

      if ( $this->verbose )
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

      $size       = str_pad( si_suffix($this->vfs_filesize( $target ), "B"), 
                                10, " ", STR_PAD_LEFT );

      $hardlinked = $this->vfs_ishardlinked( $target ) ? "*" : "";

      $this->tee( "    {$sub} {$size} {$hardlinked}{$target}", $eol );
   }


   private function vfs_addremote( $target )
   {
      $hash = $this->vfs_filehash( $target );
      $size = $this->vfs_filesize( $target );

      $this->_notquiet_report( $target, "" );

      if ( $this->sys_local->keeplocal )
      {
         $rel_ref = dir_fn( $this->current_subfolder . "/" . $target );
         $this->xfer_queue[] = array( $rel_ref, $hash, $size );
      }
        else $this->_remote_xferfile( $target, $hash, $size );

      if ( $this->notquiet() )
         $this->tee( "", "\n" );

      return $this->_vfs_local_cleanup( $target );
   }


   function vfs_addfile( $target, $source )
   {
      $this->_verbose_report( $target );

      $target = $this->encode_filenames( $target );

      if ( $this->sys_local->vfs_addfile( $target, $source ) )
         $this->fatal( "unable to add {$target} to the local backupset" );

      return $this->vfs_addremote( $target );
   }


   function vfs_compress_string( $target, $source_str )
   {
      $this->_verbose_report( $target );

      $target = $this->encode_filenames( $target );

      if ( $this->sys_local->vfs_compress_string( $target, $source_str ) )
         $this->fatal( "unable to add {$target} to the local backupset" );

      return $this->vfs_addremote( $target );
   }


   function vfs_compress_command( $cmd, $target )
   {
      $this->_verbose_report( $target );

      $target = $this->encode_filenames( $target );

      if ( $this->sys_local->vfs_compress_command( $cmd, $target ) )
         $this->fatal( "unable to add {$target} to the local backupset" );

      return $this->vfs_addremote( $target );
   }


   function vfs_compress_file( $target, $source )
   {
      $this->_verbose_report( $target );

      $target = $this->encode_filenames( $target );

      if ( $this->sys_local->vfs_compress_file( $target, $source ) )
         $this->fatal( "unable to add {$target} to the local backupset" );

      return $this->vfs_addremote( $target );
   }


   function vfs_tarball_folder( $target, $source, $options = array() )
   {
      $this->_verbose_report( $target, "folder" );

      $target = $this->encode_filenames( $target );

      if ( $this->sys_local->vfs_tarball_folder( $target, $source, $options ) )
         $this->fatal( "unable to add {$target} to the local backupset" );

      $msg = $this->tar->last_error();
      if ( strpos( $msg, SYS_TAR_FILECHANGED_TEXT ) !== false )
      {
         $t = str_replace( $this->tar->tarball_ext(), ".err", $target );
         $rel_ref = dir_fn( $this->current_subfolder . "/" . $t );
         $abs_ref = dir_fn( $this->sys_local->current_path . "/" . $rel_ref );

         $msg = "tarball may contain errors; error message was:\n\n"
              . $msg
              ;

         if ( file_put_contents( $abs_ref, $msg ) === false )
            $this->fatal( "unable to write error message file: {$rel_ref}" );

         if ( chmod( $abs_ref, $this->sys_umaskfiles ) === false )
            $this->fatal( "unable to change mode on file: {$rel_ref}" );
      }

      return $this->vfs_addremote( $target );
   }
}



