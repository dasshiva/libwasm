Name
Type
Import
Function
Table
Memory
Global
Export
Start
Element
Code
Data
// This file uses a compilicated format so take note 
// First, all lines beginning with '//' are comments like this one
// Next, all lines must be 70 columns or less in size
// This file is processed by lib/genhash which is from utils/hash.c 
// The code in hash.c is designed for internal use only and hence
// does not perform any security checks to keep compilation quick
// Note well, '//' MUST be the first two characters of the line
// Any space between the first slash and the line will cause the
// code to think this wasn't a comment and it will hash it
// Next each name that you want hashed MUST be one on each line
// No two names can be on the same line
// Do not leave any extra characters on the last line after the 
// last hash name, this can cause bugs
// Also make sure you are using an editor that uses LF as its text
// terminator and NOT CRLF
// FIXME: Another weird quirk: all comments to be after the names