# FEZPak
A drag-and-drop application that creates and extracts .pak files used in FEZ.

## Creating a .pak
To create a .pak, drag-and-drop a .txt file containing the list of files to pack, one file per line, onto FEZPak.exe.

The files must be located in the current folder (if you drag and drop, it will be the folder where your .txt is located) as the .txt file.

The .pak file will be created in the current folder under the same name as the .txt file.

## Extracting a .pak
To extract a .pak, drag-and-drop the .pak onto FEZPak.exe.

A folder under the name of your .pak will be created in the current folder, the files will be placed there.

In addition, a .txt file under the same name is created in the current folder, containing a list of files in the .pak, so you can repack it without having to list all files by yourself. To repack a .pak, move the .txt file into the folder with the extracted files and drag-and-drop it onto FEZPak.exe.