# Droidcat

## Concept and Purposes

Droidcat it's RSE based tool with purposes to manager (extends to visualizer,
unpack, decode, disassembler, encode and build), analysis (checks, rate, search),
signer (verify and sign) Android Packages files.

## Purposes offered by the tool

### Manager

- Visualizer: See a top-down features like (size, creation date, author, package
name, package properties, target android version, technologies used into the 
development stage), resources used by the application like permissions requests 
and specific storage files access during the application runtime.

- Unpack: Unzip the package in a specific location, no decode-able resource are
treated in this stage.

## Similar Projects

- Apktool (Wrote in Java)

## Program Work Situation and Execution Time

- Execution directory: Droidcat will checks firstly the installation directory
used into build parameters (parameter used at the build time), if the work 
tree isn't found inside the directory, droidcat will search into the current
runtime directory for your all program directory.

## Client and Server Execution Perspective

- Droidcat has 2 modes of operations, client and server, the client acts
as the default operation when the ```-daemon``` isn't specified at the command 
line parameters, when in server mode, Droidcat will receive communication 
from others Droidcat clients for data processing purposes, all information 
will be compressed and encrypted before any data transference occurs.
