/* Tumblr Client */
/* by Ian Brunton <iandbrunton@gmail.com> */
/* <2011-07-02 Sat> */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* global variables */
FILE *fr;
char editor [80];
int sent = 0;

/* data to be sent to server */
struct form {
  char email [80];
  char password [80];
  char generator [80];
  char type [80];
  char title [256];
  char tags [256];
  char *data;
} post_content;

/* functions */
void showMenu ();

int main (int argc, char *argv[])
{
  /* get config filename */
  char *config_dir = getenv ("XDG_CONFIG_HOME");
  char *rc_file_name = "/tumblrrc";
  size_t len1 = strlen (config_dir);
  size_t len2 = strlen (rc_file_name);
  char *rc_file = malloc (len1 + len2 + 2);
  memcpy (rc_file, config_dir, len1);
  memcpy (rc_file + len1, rc_file_name, len2 + 1);

  /* read config from rc_file */
  char line [80];
  char input_string [80];
  char field_name [80];
  char field_value [80];
  char field [80];
  int i, j;

  fr = fopen (rc_file, "r");
  while (fgets (line, 80, fr) != NULL) {
    sscanf (line, "%s", &input_string);
    for (i = 0; input_string[i] != '='; i++) {
      field_name[i] = input_string[i];
    }
    field_name[i] = '\0';
    ++i;

    for (j = 0; input_string[i] != '\0'; i++) {
      field_value[j] = input_string[i];
      ++j;
    }
    field_value[j] = '\0';
    
    if (strcmp (field_name, "email\0") == 0)
      strcpy (post_content.email, field_value);

    else if (strcmp (field_name, "password\0") == 0)
      strcpy (post_content.password, field_value);

    else if (strcmp (field_name, "editor\0") == 0)
      strcpy (editor, field_value);
  }
  fclose (fr);
  free (rc_file);

  /* printf ("\nEmail: %s\nPassword: %s\nEditor: %s\n", */
  /* 	  post_content.email, post_content.password, editor); */

  /* if editor is not set in config file, use environment variable */
  if (strlen (editor) == 0)
    strcpy (editor, getenv ("EDITOR"));

  /* Create temporary filename and prepare editor command */
  char *temp_file = tempnam ("/tmp", "tumblc");
  size_t leneditor = strlen (editor);
  size_t lentemp = strlen (temp_file);
  char *edit_command = malloc (leneditor + lentemp + 2);
  memcpy (edit_command, editor, leneditor);
  memcpy (edit_command + leneditor, " ", 1);
  memcpy (edit_command + leneditor + 1, temp_file, lentemp + 1);

  strcpy (post_content.generator, "tumblc by Ian D Brunton\0");
  strcpy (post_content.type, "text\0");

  char menu_selection = '0';

  while (menu_selection != 'q') {
    showMenu ();

    switch (menu_selection) {
    case 'e': /* edit post */
      system (edit_command);
      break;
    case 't': /* edit title */
      break;
    case 'g': /* edit tags */
      break;
    case 's': /* save draft for later */
      /* prompt for filename */
      break;
    case 'p': /* post to tumblr */
      /* read in temp file to post_content.data */
      /* uri-encode all fields */
      /* join all fields */
      /* send to tumblr */
      /* set "sent" flag */
      break;
    }
  }

  if (sent == 1) {
    /* do you want to save your post? */
  }

  free (edit_command);
  return 0;
} /* of main */

void showMenu ()
{
}
