FreeFileSync v2.1
------------------

Usage
-----
1. Choose left and right directories and "Compare" them.
2. Select "Synchronize..." to specify settings and begin synchronization.


Key Features
------------
1. Compare files (bytewise or by date) and synchronize them.
2. No limitations: An arbitrary number of files can be synchronized.
3. Unicode support.
4. Network support.
5. Full support for Windows/Linux Symbolic Links and Windows Junction Points.
6. Lean & easily accessible UI: Highly optimized for speed and huge sets of data.
7. Algorithms coded in C++ completely.
8. Progress indicators are updated only every 100ms for optimal performance!
9. Subfolders are also synchronized, including empty folders.
10. Support for multiple folder pairs
11. Create Batch Jobs for automated synchronization with or without GUI.
12. Focus on usability:
	- Only necessary functionality on UI: no overloaded menus or icon jungle.
	- Select folders via drag & drop.
	- Last configuration and screen settings are saved automatically.
	- Maintain and load different configurations by drag&drop, load-button or during startup.
	- Double-click to show file in explorer. (Windows only)
	- Copy & paste support to export file-lists.
	- Delete superfluous/temporary files directly on main grid.
	- Right-click context menu.
	- Status information and error reporting
	- Sort file-lists by name, size or date.
	- Display statistical data: total filesizes, amount of bytes that will be transfered with the current settings.
13. Support for filesizes larger than 4 GB.
14. Option to move files to Recycle Bin instead of deleting/overwriting them.
15. Automatically ignore directories "\RECYCLER" and "\System Volume Information" when comparing and sync'ing. (Windows only)
16. Localized versions for many languages are available.
17. Delete before copy: Avoid disc space shortages with large sync-operations.
18. Based on wxWidgets framework => Portable to many operating systems.
19. Filter functionality to include/exclude files from synchronization (without re-compare!).
20. Include/exclude specific files from synchronization manually.
21. Create sync jobs via GUI to synchronize automatically (can be scheduled or executed directly).
22. Handle daylight saving time changes on FAT/FAT32 volumes correctly.
23. Portable version (.zip) available.
24. No Windows registry entries for portable version.
25. Support for \\?\ path prefix for unrestricted path length. (Windows only)
26. Check for updates from within FreeFileSync automatically.
27. Copy locked files using Windows Volume Shadow Copy. (Windows only)
28. Load file icons asynchronously for maximum display performance.


Advanced topics
---------------
1.) Synchronize in Batch Mode and send error notification via email:

- Create a FreeFileSync batch file using "silent mode".
- Set error handling to "Exit with Returncode < 0" or "ignore errors" to avoid having a popup stop the program flow.
  In case errors occur FreeFileSync will abort with a returncode < 0 which can be checked via the ERRORLEVEL command.
- Create a *.cmd or *.bat file and specify the location of FreeFileSync.exe and pass the name of the FreeFileSync batch file as %1 parameter; e.g.:

	C:\Program Files\FreeFileSync\FreeFileSync.exe C:\SyncJob.ffs_batch
	IF NOT ERRORLEVEL 0 echo An error occurred! && pause

- Instead of displaying "An error occurred!" you can specify any other command like sending an email notification (using a third party tool).


2.)	Schedule Batch Job in Windows Task Planner

- Create a FreeFileSync batch file. (E.g. C:\SyncJob.ffs_batch)
- Create a new task in Windows Task planner for FreeFileSync.exe.
- Modify the task and adapt the execute-command specifying the path to the batch file; e.g.:
		
		C:\Program Files\FreeFileSync\FreeFileSync.exe C:\SyncJob.ffs_batch

		
3.) Drag & drop support

FreeFileSync has a big focus on usability. Therefore drag & drop is supported in various situations:

You can: - drag & drop any directory onto the main window to set the directory for comparison
         - drag & drop any file onto the main window to set the directory for comparison
         - drag & drop *.ffs_gui files onto the main window to load the configuration contained
         - drag & drop *.ffs_batch files onto the main window to display and edit the batch configuration
         - drag & drop *.ffs_batch files onto the batch dialog to display and edit the batch configuration

		 
4.) Synchronize two folders but exclude all subfolders from synchronization

Assuming you plan to synchronize two folders "C:\Source" and "D:\Target", you'll want to setup a filter like this:

	Include: *
	Exclude: *\*
	
This will exclude all files and folders within the two directories that contain a "\" character. These are exactly the subfolders and -files you want to exclude.


5.) Synchronize with FTP

FreeFileSync does not support FTP directly. But the FTP functionality can be easily activated by mapping the FTP webspace to a drive letter:

Example: Use the free utility NetDrive (http://www.netdrive.net/)
- Add a "New Site" and specify site name, site URL, drive letter, account and password.
- Use the newly created drive as if it were a regular hard disk.


Links
------
FreeFileSync on SourceForge:

http://sourceforge.net/projects/freefilesync/


Contact 
-------
For feedback, suggestions or bug-reports you can write an email to:
zhnmju123 [at] gmx [dot] de

or report directly to:
http://sourceforge.net/projects/freefilesync/


Have fun!
-ZenJu
