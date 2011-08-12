/* tumblc, a command-line client for Tumblr, written in C */
/* by Ian Brunton <iandbrunton@gmail.com> */
/* <2011-07-02 Sat> */

#define VERSION "0.1a"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <time.h>
#include <errno.h>

#include <curl/curl.h>

/* global variables */
FILE *fr;
char editor [80];
int sent = 0;
char tumblrapi [] = "http://www.tumblr.com/api/write";
char *draft_filename;
char *draft_directory;
char *temp_file;

/* curl */
CURL *curl;
CURLcode res;
struct curl_httppost *formpost=NULL;
struct curl_httppost *lastptr=NULL;
struct curl_slist *headerlist=NULL;
static const char buf[] = "Expect:";

/* data to be sent to server */
struct form {
  char email [80];
  char password [80];
  char generator [80];
  char type [80];
  char state [16];
  char title [256];
  char tags [256];
  char *body;
} post_content;

/*for reading directories*/
typedef struct filenode {
  char *name;
  size_t size;
  int index;
  struct filenode *next;
} filenode;

/* functions */
void showMenu ();
int saveDraft ();
int loadDraft ();
int sendPost ();

int main (int argc, char *argv[])
{
 
  printf ("--------------[ tumblc %s, a Tumblr command-line client ]--------------\n\n",
	  VERSION);

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

  /* if editor is not set in config file, use environment variable */
  if (strlen (editor) == 0)
    strcpy (editor, getenv ("EDITOR"));

  /* Create temporary filename and prepare editor command */
  temp_file = tempnam ("/tmp", "tumblc");
  size_t leneditor = strlen (editor);
  size_t lentemp = strlen (temp_file);
  char *edit_command = malloc (leneditor + lentemp + 2);
  memcpy (edit_command, editor, leneditor);
  memcpy (edit_command + leneditor, " ", 1);
  memcpy (edit_command + leneditor + 1, temp_file, lentemp + 1);

  strcpy (post_content.generator, "tumblc by Ian D Brunton\0");
  strcpy (post_content.type, "regular\0");
  strcpy (post_content.state, "draft\0");

  /* Get menu selection & act accordingly */
  int menuin;
  char menu_selection;

  int ti = 0;

  while (menu_selection != 'q') {
    showMenu ();
    printf ("Enter choice, and press return: ");
    while ((menuin = getchar ()) != '\n')
      menu_selection = (char)menuin;

    switch (menu_selection) {
    case 'e': /* edit post */
      system (edit_command);
      break;

    case 't': /* edit title */
      printf ("Title: ");
      ti = 0;
      while ((menuin = getchar ()) != '\n') {
	  	post_content.title[ti] = (char)menuin;
	  	++ti;
      }
      post_content.title[ti] = '\0';
      break;

    case 'g': /* edit tags */
      printf ("Tags: ");
      ti = 0;
      while ((menuin = getchar ()) != '\n') {
	  	post_content.tags[ti] = (char)menuin;
	  	++ti;
      }
      post_content.tags[ti] = '\0';
      break;

    case 'a': /* edit state */
      printf ("State (draft, published): ");
      ti = 0;
      while ((menuin = getchar ()) != '\n') {
	  	post_content.state[ti] = (char)menuin;
	  	++ti;
      }
      post_content.state[ti] = '\0';
      break;

    case 'r': /* review post */
      /* clear screen... */
      printf ("Your post:\n");
      printf ("Title: %s\n", post_content.title);
      printf ("Tags: %s\n", post_content.tags);
      printf ("\n\n");
      /* send to PAGER */
      break;

    case 's': /* save draft for later */
      saveDraft ();
      break;

	case 'l': /* load previously saved draft */
	  loadDraft ();
	  break;

    case 'p': /* post to tumblr */
      if (sendPost () == 0) {
		sent = 1;
	  }
	  else {
		sent = 0;
		printf ("Post failed.\n");
		printf ("[s] Save draft or [p] Try again");
	  }
      break;

    case 'q': /* quit */
      break;

    default:
      printf ("Invalid option.\n");
      break;
    }

  }

  /* 'q' for Quit, but post not yet sent */
  if (sent == 0) { 
    printf ("Draft is not empty. Do you want to save it? [Y/n] ");
	char save_choice;
	while ((menuin = getchar ()) != '\n')
	  save_choice = (char)menuin;
	
    if (save_choice == 'Y' || save_choice == 'y')
      saveDraft ();
  }

  free (edit_command);
  free (post_content.body);
  /* delete temp file */
  remove (temp_file);
  printf ("\nThank you for using tumblc.\n\n");
  return 0;
} /* of main */

void showMenu ()
{
  printf ("MAIN MENU:\n\n");
  printf ("[e] Edit post\n");
  printf ("[t] Edit title\n");
  printf ("[g] Edit tags\n");
  printf ("[a] Edit post state\n");
  printf ("[r] Review post\n");
  printf ("[s] Save post as draft\n");
  printf ("[l] Load post from draft\n");
  printf ("[p] Send post to Tumblr\n");
  printf ("[q] Quit\n\n");
  return;
}

int saveDraft ()
{
  /* set up draft dir */
  char *xdg_data = getenv ("XDG_DATA_HOME");
  char *draft_dir = "/tumblc/";
  char *draft_location = "/tumblc/draft";

  char draft_str [15]; /* YYYYMMDDHHMMSS: "20110706120732\0" */
  time_t time_raw_format;
  struct tm * time_struct;
  time (&time_raw_format);
  time_struct = localtime (&time_raw_format);
  strftime (draft_str, 16, "%Y%m%d%H%M%S", time_struct);

  size_t lenxdgdata = strlen (xdg_data);
  size_t lendraftdir = strlen (draft_dir);
  size_t lendraftloc = strlen (draft_location);
  size_t lendraftstr = strlen (draft_str);

  draft_directory = malloc (lenxdgdata + lendraftdir + 2);
  memcpy (draft_directory, xdg_data, lenxdgdata);
  memcpy (draft_directory + lenxdgdata, draft_dir, lendraftdir + 1);

  draft_filename = malloc (lenxdgdata + lendraftloc + lendraftstr + 3);
  memcpy (draft_filename, xdg_data, lenxdgdata);
  memcpy (draft_filename + lenxdgdata, draft_location, lendraftloc);
  memcpy (draft_filename + lenxdgdata + lendraftloc, draft_str, lendraftstr + 1);

  printf ("Saving draft to file `%s'...", draft_filename);

  FILE *fw;
  fw = fopen (draft_filename, "w");
  if (fw == 0) {
	fprintf (stderr, "Could not open file `%s'.\n", draft_filename);
	return 1;
  }
  fprintf (fw, "Title: %s\n", post_content.title);
  fprintf (fw, "Tags: %s\n", post_content.tags);
  fprintf (fw, "State: %s\n", post_content.state);
  fprintf (fw, "--------\n");

  FILE *ft;
  ft = fopen (temp_file, "r");
  if (ft == 0) {
	fprintf (stderr, "Could not open file `%s'.\n", temp_file);
	return 1;
  }
  int tempc;
  while ((tempc = fgetc(ft)) != EOF) {
	fputc (tempc, fw);
	/*fprintf (fw, "%c", (char)tempc);*/
  }
  close (ft);
  fclose (fw);
  printf (" Draft saved.\n");

  return 0;
}

int loadDraft ()
{
  filenode* firstFileNode = NULL;
  filenode* newFileNode = malloc (sizeof (filenode));
  if (newFileNode == NULL) {
	fprintf (stderr, "Error allocating memory.\n");
	return 1;
  }

  if (firstFileNode == NULL)
	firstFileNode = newFileNode;

  /*get list of existing drafts*/
  filenode* fileWalker = firstFileNode;

  DIR *dir;
  FILE *fd;
  int file_count = 0;
  struct dirent *ent;
  struct stat *fileattr = malloc (sizeof (struct stat));

  dir = opendir (draft_directory);
  while ((ent = readdir (dir)) != NULL) {
	++file_count;
	if (stat (end->d_name, fileattr) == 0) {
	  if (S_ISDIR (fileattr->st_mode) != 0)
		continue; /* skip directories */

	  strcpy (fileWalker->name, ent->d_name);
	  fileWalker->size = fileattr->st_size;
	  fileWalker->index = file_count;
	  fileWalker->next = malloc (sizeof (filenode));
	  fileWalker = fileWalker->next;
	}
	else {
	  fprintf (stderr, "Cannot stat file %s: %s\n", fileWalker->name,
		  strerror (errno));
	  break;
	}
  }
  closedir (dir);
  free (fileattr);

  /*present list, enumerated*/
  printf ("%d draft files found.\n", file_count);

  fileWalker = firstfileNode;
  while (fileWalker->next != NULL) {
	printf ("%d\t%s (%d bytes)\n", fileWalker->index, fileWalker->name,
		fileWalker->file_size);
  }

  printf ("Enter file number: ");
  int selection;
  scanf ("%d", &selection);

  /*selected file to draft_filename*/
  fileWalker = firstFileNode;
  while (fileWalker->next != NULL) {
	if (fileWalker->index != selection) {
	  strcpy (draft_filename, fileWalker->name);
	  break;
	}
	else
	  fileWalker = fileWalker->next;
  }

  /*read file*/
  FILE *fd;
  fd = fopen (draft_filename);
  if (fd == 0) {
	fprintf (stderr, "Error opening file `%s': %s\n", draft_filename, strerror (errno));
	return 1;
  }
  char *buff = malloc (fileWalker->size);;
  int dc, ci = 0;
  while ((dc = fgetc (fd)) != EOF) {
	buff[ci] = dc;
	++ci;
  }

  /*parse file contents into variables*/
  ci = 0;
  char buf [80];

  return 0;
}

int sendPost ()
{
  FILE *fp;
  int c;
  int bi = 0;
  size_t file_size;

  fp = fopen (temp_file, "rb");
  if (fp == 0)
    fprintf (stderr,"Could not open file `%s'.\n", temp_file);

  fseek (fp, 0, SEEK_END);
  file_size = ftell (fp);
  fclose (fp);
  post_content.body = malloc (file_size + 1);

  fp = fopen (temp_file, "r");
  while ((c = fgetc (fp)) != EOF) {
    post_content.body[bi] = (char)c;
    ++bi;
  }
  post_content.body[bi] = '\0';
  fclose (fp);

  /* take care of defaults */
  if (strlen (post_content.state) < 2)
    strcpy (post_content.state, "published\0");

  curl_global_init (CURL_GLOBAL_ALL);
  
  curl_formadd (&formpost,
  		&lastptr,
  		CURLFORM_COPYNAME, "email",
  		CURLFORM_COPYCONTENTS, post_content.email,
  		CURLFORM_END);
  
  curl_formadd (&formpost,
  		&lastptr,
  		CURLFORM_COPYNAME, "password",
  		CURLFORM_COPYCONTENTS, post_content.password,
  		CURLFORM_END);
  
  curl_formadd (&formpost,
  		&lastptr,
  		CURLFORM_COPYNAME, "generator",
  		CURLFORM_COPYCONTENTS, post_content.generator,
  		CURLFORM_END);
  
  curl_formadd (&formpost,
  		&lastptr,
  		CURLFORM_COPYNAME, "type",
  		CURLFORM_COPYCONTENTS, post_content.type,
  		CURLFORM_END);
  
  curl_formadd (&formpost,
  		&lastptr,
  		CURLFORM_COPYNAME, "title",
  		CURLFORM_COPYCONTENTS, post_content.title,
  		CURLFORM_END);
  
  curl_formadd (&formpost,
  		&lastptr,
  		CURLFORM_COPYNAME, "tags",
  		CURLFORM_COPYCONTENTS, post_content.tags,
  		CURLFORM_END);
  
  curl_formadd (&formpost,
  		&lastptr,
  		CURLFORM_COPYNAME, "state",
  		CURLFORM_COPYCONTENTS, post_content.state,
  		CURLFORM_END);
  
  curl_formadd (&formpost,
  		&lastptr,
  		CURLFORM_COPYNAME, "body",
  		CURLFORM_COPYCONTENTS, post_content.body,
  		CURLFORM_END);
  
  curl = curl_easy_init ();
  
  headerlist = curl_slist_append (headerlist, buf);
  if (curl) {
    curl_easy_setopt (curl, CURLOPT_URL, tumblrapi);
    /* if ((argc == 2) && (!strcmp (argv[1], "noexpectheader")) ) */
    /*   /\* only disable 100-continue header if explicitly requested *\/  */
    /*   curl_easy_setopt (curl, CURLOPT_HTTPHEADER, headerlist); */
    curl_easy_setopt (curl, CURLOPT_HTTPPOST, formpost);
    res = curl_easy_perform (curl);

	if (res == 0) { /* success */
      /* always cleanup */
      curl_easy_cleanup (curl);
    
      /* then cleanup the formpost chain */
      curl_formfree (formpost);
      /* free slist */
      curl_slist_free_all (headerlist);
  	}
  }
  return res;
}
