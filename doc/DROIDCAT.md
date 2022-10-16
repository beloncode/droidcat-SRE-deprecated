# Droidcat

## Concept and Purposes

Droidcat it's RSE based tool with purposes to manager (extends to visualizer, unpack, decode, disassembler, encode and build), analysis (checks, rate, search), signer (verify and sing) Android Packages files.

## Purposes offered by the tool

### Manager

- Visualizer: See a top-down features like (size, creation date, author, package name, package properties, target android version, technologies used into the development stage), resources used by the application like permissions requests and specific storage files access during the application runtime.

- Unpack: Unzip the package in a specific location, no decode-able resource are treated in this stage.

## Similar Projects

- Apktool (Wrote in Java)

## Program Work Situation and Execution Time

- Execution directory: Droidcat will checks firstly the installation directory used into build parameters (parameter used at the build time), if the work tree isn't found inside the directory, droidcat will search into the current runtime directory for your all program directory.
 
