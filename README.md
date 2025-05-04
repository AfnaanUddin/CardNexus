vCard Parser and Validator - README
Overview
--------
The vCard Parser and Validator is a C-based project that provides functionality to manage,
manipulate, and validate vCard data in the .vcf or .vcard format. This project includes functions to
write, validate, and convert vCard data, as well as operations on DateTime and other vCard
properties.
This project is designed to provide methods for handling vCard files that adhere to version 4.0
specifications. It supports basic vCard properties like FN, BDAY, ANNIVERSARY, and allows for
custom optional properties. The project also includes methods for managing DateTime fields, which
can be either text-based or structured.
Features
--------
- vCard Parsing: Ability to read and parse vCard data and properties.
- vCard Writing: Write valid vCard data to .vcf or .vcard files.
- DateTime Handling: Support for both structured and text-based DateTime fields.
- vCard Validation: Ensure the validity of a vCard by checking required fields and ensuring
correctness in optional fields like KIND, BDAY, and ANNIVERSARY.
- Memory Management: Functions for handling memory dynamically (e.g., using malloc, free, and
strdup for data storage).
Project Structure
-----------------
The project contains the following key functions and components:
- writeCard
- validateCard
- dateToString
- deleteDate
- compareDates
- valueToString
Error Codes
------------
- OK: The operation was successful.
- WRITE_ERROR: There was an error writing to the file.
- INV_FILE: The provided file is invalid or doesn't have a valid .vcf or .vcard extension.
- INV_CARD: The provided vCard is invalid.
- INV_PROP: An invalid property was found in the vCard.
- INV_DT: The DateTime fields in the vCard are invalid.
Python UI Component
-------------------
The project also includes a Python user interface (UI) that allows users to interact with the vCard
data visually. Using libraries such as `tkinter` or `PyQt`, the UI provides easy navigation for
importing, parsing, and validating vCards. The Python UI enables the user to:
- Load vCard files.
- View vCard details in a user-friendly format.
- Validate and display error messages if the vCard is invalid.
Database Component
------------------
The project integrates an SQL database to store parsed vCard data. Using `SQLite` or a similar
database management system, the database allows:
- Storing parsed vCard information in structured tables.
- Retrieving vCard details from the database for further processing or validation.
- Performing SQL queries to filter and search for specific vCard entries.
Example Usage
-------------
Writing a vCard to a file:
```c
Card *card = createCard();
writeCard("contact.vcf", card);
```
Validating a vCard:
```c
Card *card = createCard();
VCardErrorCode result = validateCard(carif (result == OK) {
 printf("vCard is valid.
");
} else {
 printf("vCard is invalid.
");
}
```
Converting DateTime to string:
```c
DateTime *dt = createDateTime();
char *dateStr = dateToString(dt);
printf("%s
", dateStr);
free(dateStr);
```
Freeing DateTime:
```c
deleteDate(dt);
```
Dependencies
------------
- C Standard Library (stdio.h, stdlib.h, string.h)
- Python libraries (tkinter or PyQt for UI, sqlite3 for database)
License
-------
This project is licensed under the MIT License - see the LICENSE file for details.
Author
------
**Mohammed Afnaan Uddin**
