#include <el/ext.h>

//!!! not only getopt

extern "C" {



int	opterr = 1,		/* if error message should be printed */
	optind = 1,		/* index into parent argv vector */
	optopt,			/* character checked for validity */
	optreset;		/* reset getopt */
char	*optarg;		/* argument associated with option */

#define	BADCH	(int)'?'
#define	BADARG	(int)':'
#define	EMSG	""

/*
 * getopt --
 *	Parse argc/argv argument vector.
 */
int AFXAPI getopt(int nargc, char * const *nargv, const char *ostr)
{
	char *__progname="progname";
	static char *place = EMSG;		/* option letter processing */
	const char *oli;				/* option letter list index */

	if (optreset || !*place) {		/* update scanning pointer */
		optreset = 0;
		if (optind >= nargc || *(place = nargv[optind]) != '-') {
			place = EMSG;
			return (-1);
		}
		if (place[1] && *++place == '-') {	/* found "--" */
			++optind;
			place = EMSG;
			return (-1);
		}
	}					/* option letter okay? */
	if ((optopt = (int)*place++) == (int)':' ||
	    !(oli = strchr(ostr, optopt))) {
		/*
		 * if the user didn't specify '-' as an option,
		 * assume it means -1.
		 */
		if (optopt == (int)'-')
			return (-1);
		if (!*place)
			++optind;
		if (opterr && *ostr != ':')
			(void)fprintf(stderr,
			    "%s: illegal option -- %c\n", __progname, optopt);
		return (BADCH);
	}
	if (*++oli != ':') {			/* don't need argument */
		optarg = NULL;
		if (!*place)
			++optind;
	}
	else {					/* need an argument */
		if (*place)			/* no white space */
			optarg = place;
		else if (nargc <= ++optind) {	/* no arg */
			place = EMSG;
			if (*ostr == ':')
				return (BADARG);
			if (opterr)
				(void)fprintf(stderr,
				    "%s: option requires an argument -- %c\n",
				    __progname, optopt);
			return (BADCH);
		}
	 	else				/* white space */
			optarg = nargv[optind];
		place = EMSG;
		++optind;
	}
	return (optopt);			/* dump back option letter */
}


int __getopt_initialized;

static int first_nonopt;
static int last_nonopt;
static char *nextchar;
static char *posixly_correct;

static enum
{
  REQUIRE_ORDER, PERMUTE, RETURN_IN_ORDER
} ordering;


static const char *_getopt_initialize (
     int argc,
     char *const *argv,
     const char *optstring)
{
  /* Start processing options with ARGV-element 1 (since ARGV-element 0
     is the program name); the sequence of previously skipped
     non-option ARGV-elements is empty.  */

  first_nonopt = last_nonopt = optind;

  nextchar = NULL;

#if !UCFG_WCE
  posixly_correct = getenv ("POSIXLY_CORRECT");
#endif

  /* Determine how to handle the ordering of options and nonoptions.  */

  if (optstring[0] == '-')
    {
      ordering = RETURN_IN_ORDER;
      ++optstring;
    }
  else if (optstring[0] == '+')
    {
      ordering = REQUIRE_ORDER;
      ++optstring;
    }
  else if (posixly_correct != NULL)
    ordering = REQUIRE_ORDER;
  else
    ordering = PERMUTE;

#if defined _LIBC && defined USE_NONOPTION_FLAGS
  if (posixly_correct == NULL
      && argc == __libc_argc && argv == __libc_argv)
    {
      if (nonoption_flags_max_len == 0)
	{
	  if (__getopt_nonoption_flags == NULL
	      || __getopt_nonoption_flags[0] == '\0')
	    nonoption_flags_max_len = -1;
	  else
	    {
	      const char *orig_str = __getopt_nonoption_flags;
	      int len = nonoption_flags_max_len = strlen (orig_str);
	      if (nonoption_flags_max_len < argc)
		nonoption_flags_max_len = argc;
	      __getopt_nonoption_flags =
		(char *) malloc (nonoption_flags_max_len);
	      if (__getopt_nonoption_flags == NULL)
		nonoption_flags_max_len = -1;
	      else
		memset (__mempcpy (__getopt_nonoption_flags, orig_str, len),
			'\0', nonoption_flags_max_len - len);
	    }
	}
      nonoption_flags_len = nonoption_flags_max_len;
    }
  else
    nonoption_flags_len = 0;
#endif

  return optstring;
}

# define SWAP_FLAGS(ch1, ch2)

static void exchange (char **argv)
{
  int bottom = first_nonopt;
  int middle = last_nonopt;
  int top = optind;
  char *tem;

  /* Exchange the shorter segment with the far end of the longer segment.
     That puts the shorter segment into the right place.
     It leaves the longer segment in the right place overall,
     but it consists of two parts that need to be swapped next.  */

#if defined _LIBC && defined USE_NONOPTION_FLAGS
  /* First make sure the handling of the `__getopt_nonoption_flags'
     string can work normally.  Our top argument must be in the range
     of the string.  */
  if (nonoption_flags_len > 0 && top >= nonoption_flags_max_len)
    {
      /* We must extend the array.  The user plays games with us and
	 presents new arguments.  */
      char *new_str = malloc (top + 1);
      if (new_str == NULL)
	nonoption_flags_len = nonoption_flags_max_len = 0;
      else
	{
	  memset (__mempcpy (new_str, __getopt_nonoption_flags,
			     nonoption_flags_max_len),
		  '\0', top + 1 - nonoption_flags_max_len);
	  nonoption_flags_max_len = top + 1;
	  __getopt_nonoption_flags = new_str;
	}
    }
#endif

  while (top > middle && middle > bottom)
    {
      if (top - middle > middle - bottom)
	{
	  /* Bottom segment is the short one.  */
	  int len = middle - bottom;
	  int i;

	  /* Swap it with the top part of the top segment.  */
	  for (i = 0; i < len; i++)
	    {
	      tem = argv[bottom + i];
	      argv[bottom + i] = argv[top - (middle - bottom) + i];
	      argv[top - (middle - bottom) + i] = tem;
	      SWAP_FLAGS (bottom + i, top - (middle - bottom) + i);
	    }
	  /* Exclude the moved bottom segment from further swapping.  */
	  top -= len;
	}
      else
	{
	  /* Top segment is the short one.  */
	  int len = top - middle;
	  int i;

	  /* Swap it with the bottom part of the bottom segment.  */
	  for (i = 0; i < len; i++)
	    {
	      tem = argv[bottom + i];
	      argv[bottom + i] = argv[middle + i];
	      argv[middle + i] = tem;
	      SWAP_FLAGS (bottom + i, middle + i);
	    }
	  /* Exclude the moved top segment from further swapping.  */
	  bottom += len;
	}
    }

  /* Update records for the slots the non-options now occupy.  */

  first_nonopt += (optind - last_nonopt);
  last_nonopt = optind;
}

# define my_index	strchr
#  define _(msgid)	(msgid)

int
_getopt_internal (
	 int argc,
     char *const *argv,
     const char *optstring,
     const struct option *longopts,
     int *longind,
     int long_only)
{
  int print_errors = opterr;
  if (optstring[0] == ':')
    print_errors = 0;

  if (argc < 1)
    return -1;

  optarg = NULL;

  if (optind == 0 || !__getopt_initialized)
    {
      if (optind == 0)
	optind = 1;	/* Don't scan ARGV[0], the program name.  */
      optstring = _getopt_initialize (argc, argv, optstring);
      __getopt_initialized = 1;
    }

  /* Test whether ARGV[optind] points to a non-option argument.
     Either it does not have option syntax, or there is an environment flag
     from the shell indicating it is not an option.  The later information
     is only used when the used in the GNU libc.  */
#if defined _LIBC && defined USE_NONOPTION_FLAGS
# define NONOPTION_P (argv[optind][0] != '-' || argv[optind][1] == '\0'	      \
		      || (optind < nonoption_flags_len			      \
			  && __getopt_nonoption_flags[optind] == '1'))
#else
# define NONOPTION_P (argv[optind][0] != '-' || argv[optind][1] == '\0')
#endif

  if (nextchar == NULL || *nextchar == '\0')
    {
      /* Advance to the next ARGV-element.  */

      /* Give FIRST_NONOPT & LAST_NONOPT rational values if OPTIND has been
	 moved back by the user (who may also have changed the arguments).  */
      if (last_nonopt > optind)
	last_nonopt = optind;
      if (first_nonopt > optind)
	first_nonopt = optind;

      if (ordering == PERMUTE)
	{
	  /* If we have just processed some options following some non-options,
	     exchange them so that the options come first.  */

	  if (first_nonopt != last_nonopt && last_nonopt != optind)
	    exchange ((char **) argv);
	  else if (last_nonopt != optind)
	    first_nonopt = optind;

	  /* Skip any additional non-options
	     and extend the range of non-options previously skipped.  */

	  while (optind < argc && NONOPTION_P)
	    optind++;
	  last_nonopt = optind;
	}

      /* The special ARGV-element `--' means premature end of options.
	 Skip it like a null option,
	 then exchange with previous non-options as if it were an option,
	 then skip everything else like a non-option.  */

      if (optind != argc && !strcmp (argv[optind], "--"))
	{
	  optind++;

	  if (first_nonopt != last_nonopt && last_nonopt != optind)
	    exchange ((char **) argv);
	  else if (first_nonopt == last_nonopt)
	    first_nonopt = optind;
	  last_nonopt = argc;

	  optind = argc;
	}

      /* If we have done all the ARGV-elements, stop the scan
	 and back over any non-options that we skipped and permuted.  */

      if (optind == argc)
	{
	  /* Set the next-arg-index to point at the non-options
	     that we previously skipped, so the caller will digest them.  */
	  if (first_nonopt != last_nonopt)
	    optind = first_nonopt;
	  return -1;
	}

      /* If we have come to a non-option and did not permute it,
	 either stop the scan or describe it to the caller and pass it by.  */

      if (NONOPTION_P)
	{
	  if (ordering == REQUIRE_ORDER)
	    return -1;
	  optarg = argv[optind++];
	  return 1;
	}

      /* We have found another option-ARGV-element.
	 Skip the initial punctuation.  */

      nextchar = (argv[optind] + 1
		  + (longopts != NULL && argv[optind][1] == '-'));
    }

  /* Decode the current option-ARGV-element.  */

  /* Check whether the ARGV-element is a long option.

     If long_only and the ARGV-element has the form "-f", where f is
     a valid short option, don't consider it an abbreviated form of
     a long option that starts with f.  Otherwise there would be no
     way to give the -f short option.

     On the other hand, if there's a long option "fubar" and
     the ARGV-element is "-fu", do consider that an abbreviation of
     the long option, just like "--fu", and not "-f" with arg "u".

     This distinction seems to be the most useful approach.  */

  if (longopts != NULL
      && (argv[optind][1] == '-'
	  || (long_only && (argv[optind][2] || !my_index (optstring, argv[optind][1])))))
    {
      char *nameend;
      const struct option *p;
      const struct option *pfound = NULL;
      int exact = 0;
      int ambig = 0;
      int indfound = -1;
      int option_index;

      for (nameend = nextchar; *nameend && *nameend != '='; nameend++)
	/* Do nothing.  */ ;

      /* Test all long options for either exact match
	 or abbreviated matches.  */
      for (p = longopts, option_index = 0; p->name; p++, option_index++)
	if (!strncmp (p->name, nextchar, nameend - nextchar))
	  {
	    if ((unsigned int) (nameend - nextchar)
		== (unsigned int) strlen (p->name))
	      {
		/* Exact match found.  */
		pfound = p;
		indfound = option_index;
		exact = 1;
		break;
	      }
	    else if (pfound == NULL)
	      {
		/* First nonexact match found.  */
		pfound = p;
		indfound = option_index;
	      }
	    else if (long_only
		     || pfound->has_arg != p->has_arg
		     || pfound->flag != p->flag
		     || pfound->val != p->val)
	      /* Second or later nonexact match found.  */
	      ambig = 1;
	  }

      if (ambig && !exact)
	{
	  if (print_errors)
	    {
#if defined _LIBC && defined USE_IN_LIBIO
	      char *buf;

	      if (__asprintf (&buf, _("%s: option `%s' is ambiguous\n"),
			      argv[0], argv[optind]) >= 0)
		{

		  if (_IO_fwide (stderr, 0) > 0)
		    __fwprintf (stderr, L"%s", buf);
		  else
		    fputs (buf, stderr);

		  free (buf);
		}
#else
	      fprintf (stderr, _("%s: option `%s' is ambiguous\n"),
		       argv[0], argv[optind]);
#endif
	    }
	  nextchar += strlen (nextchar);
	  optind++;
	  optopt = 0;
	  return '?';
	}

      if (pfound != NULL)
	{
	  option_index = indfound;
	  optind++;
	  if (*nameend)
	    {
	      /* Don't test has_arg with >, because some C compilers don't
		 allow it to be used on enums.  */
	      if (pfound->has_arg)
		optarg = nameend + 1;
	      else
		{
		  if (print_errors)
		    {
#if defined _LIBC && defined USE_IN_LIBIO
		      char *buf;
		      int n;
#endif

		      if (argv[optind - 1][1] == '-')
			{
			  /* --option */
#if defined _LIBC && defined USE_IN_LIBIO
			  n = __asprintf (&buf, _("\
%s: option `--%s' doesn't allow an argument\n"),
					  argv[0], pfound->name);
#else
			  fprintf (stderr, _("\
%s: option `--%s' doesn't allow an argument\n"),
				   argv[0], pfound->name);
#endif
			}
		      else
			{
			  /* +option or -option */
#if defined _LIBC && defined USE_IN_LIBIO
			  n = __asprintf (&buf, _("\
%s: option `%c%s' doesn't allow an argument\n"),
					  argv[0], argv[optind - 1][0],
					  pfound->name);
#else
			  fprintf (stderr, _("\
%s: option `%c%s' doesn't allow an argument\n"),
				   argv[0], argv[optind - 1][0], pfound->name);
#endif
			}

#if defined _LIBC && defined USE_IN_LIBIO
		      if (n >= 0)
			{
			  if (_IO_fwide (stderr, 0) > 0)
			    __fwprintf (stderr, L"%s", buf);
			  else
			    fputs (buf, stderr);

			  free (buf);
			}
#endif
		    }

		  nextchar += strlen (nextchar);

		  optopt = pfound->val;
		  return '?';
		}
	    }
	  else if (pfound->has_arg == 1)
	    {
	      if (optind < argc)
		optarg = argv[optind++];
	      else
		{
		  if (print_errors)
		    {
#if defined _LIBC && defined USE_IN_LIBIO
		      char *buf;

		      if (__asprintf (&buf, _("\
%s: option `%s' requires an argument\n"),
				      argv[0], argv[optind - 1]) >= 0)
			{
			  if (_IO_fwide (stderr, 0) > 0)
			    __fwprintf (stderr, L"%s", buf);
			  else
			    fputs (buf, stderr);

			  free (buf);
			}
#else
		      fprintf (stderr,
			       _("%s: option `%s' requires an argument\n"),
			       argv[0], argv[optind - 1]);
#endif
		    }
		  nextchar += strlen (nextchar);
		  optopt = pfound->val;
		  return optstring[0] == ':' ? ':' : '?';
		}
	    }
	  nextchar += strlen (nextchar);
	  if (longind != NULL)
	    *longind = option_index;
	  if (pfound->flag)
	    {
	      *(pfound->flag) = pfound->val;
	      return 0;
	    }
	  return pfound->val;
	}

      /* Can't find it as a long option.  If this is not getopt_long_only,
	 or the option starts with '--' or is not a valid short
	 option, then it's an error.
	 Otherwise interpret it as a short option.  */
      if (!long_only || argv[optind][1] == '-'
	  || my_index (optstring, *nextchar) == NULL)
	{
	  if (print_errors)
	    {
#if defined _LIBC && defined USE_IN_LIBIO
	      char *buf;
	      int n;
#endif

	      if (argv[optind][1] == '-')
		{
		  /* --option */
#if defined _LIBC && defined USE_IN_LIBIO
		  n = __asprintf (&buf, _("%s: unrecognized option `--%s'\n"),
				  argv[0], nextchar);
#else
		  fprintf (stderr, _("%s: unrecognized option `--%s'\n"),
			   argv[0], nextchar);
#endif
		}
	      else
		{
		  /* +option or -option */
#if defined _LIBC && defined USE_IN_LIBIO
		  n = __asprintf (&buf, _("%s: unrecognized option `%c%s'\n"),
				  argv[0], argv[optind][0], nextchar);
#else
		  fprintf (stderr, _("%s: unrecognized option `%c%s'\n"),
			   argv[0], argv[optind][0], nextchar);
#endif
		}

#if defined _LIBC && defined USE_IN_LIBIO
	      if (n >= 0)
		{
		  if (_IO_fwide (stderr, 0) > 0)
		    __fwprintf (stderr, L"%s", buf);
		  else
		    fputs (buf, stderr);

		  free (buf);
		}
#endif
	    }
	  nextchar = (char *) "";
	  optind++;
	  optopt = 0;
	  return '?';
	}
    }

  /* Look at and handle the next short option-character.  */

  {
    char c = *nextchar++;
    char *temp = (char*)my_index (optstring, c);

    /* Increment `optind' when we start to process its last character.  */
    if (*nextchar == '\0')
      ++optind;

    if (temp == NULL || c == ':')
      {
	if (print_errors)
	  {
#if defined _LIBC && defined USE_IN_LIBIO
	      char *buf;
	      int n;
#endif

	    if (posixly_correct)
	      {
		/* 1003.2 specifies the format of this message.  */
#if defined _LIBC && defined USE_IN_LIBIO
		n = __asprintf (&buf, _("%s: illegal option -- %c\n"),
				argv[0], c);
#else
		fprintf (stderr, _("%s: illegal option -- %c\n"), argv[0], c);
#endif
	      }
	    else
	      {
#if defined _LIBC && defined USE_IN_LIBIO
		n = __asprintf (&buf, _("%s: invalid option -- %c\n"),
				argv[0], c);
#else
		fprintf (stderr, _("%s: invalid option -- %c\n"), argv[0], c);
#endif
	      }

#if defined _LIBC && defined USE_IN_LIBIO
	    if (n >= 0)
	      {
		if (_IO_fwide (stderr, 0) > 0)
		  __fwprintf (stderr, L"%s", buf);
		else
		  fputs (buf, stderr);

		free (buf);
	      }
#endif
	  }
	optopt = c;
	return '?';
      }
    /* Convenience. Treat POSIX -W foo same as long option --foo */
    if (temp[0] == 'W' && temp[1] == ';')
      {
	char *nameend;
	const struct option *p;
	const struct option *pfound = NULL;
	int exact = 0;
	int ambig = 0;
	int indfound = 0;
	int option_index;

	/* This is an option that requires an argument.  */
	if (*nextchar != '\0')
	  {
	    optarg = nextchar;
	    /* If we end this ARGV-element by taking the rest as an arg,
	       we must advance to the next element now.  */
	    optind++;
	  }
	else if (optind == argc)
	  {
	    if (print_errors)
	      {
		/* 1003.2 specifies the format of this message.  */
#if defined _LIBC && defined USE_IN_LIBIO
		char *buf;

		if (__asprintf (&buf,
				_("%s: option requires an argument -- %c\n"),
				argv[0], c) >= 0)
		  {
		    if (_IO_fwide (stderr, 0) > 0)
		      __fwprintf (stderr, L"%s", buf);
		    else
		      fputs (buf, stderr);

		    free (buf);
		  }
#else
		fprintf (stderr, _("%s: option requires an argument -- %c\n"),
			 argv[0], c);
#endif
	      }
	    optopt = c;
	    if (optstring[0] == ':')
	      c = ':';
	    else
	      c = '?';
	    return c;
	  }
	else
	  /* We already incremented `optind' once;
	     increment it again when taking next ARGV-elt as argument.  */
	  optarg = argv[optind++];

	/* optarg is now the argument, see if it's in the
	   table of longopts.  */

	for (nextchar = nameend = optarg; *nameend && *nameend != '='; nameend++)
	  /* Do nothing.  */ ;

	/* Test all long options for either exact match
	   or abbreviated matches.  */
	for (p = longopts, option_index = 0; p->name; p++, option_index++)
	  if (!strncmp (p->name, nextchar, nameend - nextchar))
	    {
	      if ((unsigned int) (nameend - nextchar) == strlen (p->name))
		{
		  /* Exact match found.  */
		  pfound = p;
		  indfound = option_index;
		  exact = 1;
		  break;
		}
	      else if (pfound == NULL)
		{
		  /* First nonexact match found.  */
		  pfound = p;
		  indfound = option_index;
		}
	      else
		/* Second or later nonexact match found.  */
		ambig = 1;
	    }
	if (ambig && !exact)
	  {
	    if (print_errors)
	      {
#if defined _LIBC && defined USE_IN_LIBIO
		char *buf;

		if (__asprintf (&buf, _("%s: option `-W %s' is ambiguous\n"),
				argv[0], argv[optind]) >= 0)
		  {
		    if (_IO_fwide (stderr, 0) > 0)
		      __fwprintf (stderr, L"%s", buf);
		    else
		      fputs (buf, stderr);

		    free (buf);
		  }
#else
		fprintf (stderr, _("%s: option `-W %s' is ambiguous\n"),
			 argv[0], argv[optind]);
#endif
	      }
	    nextchar += strlen (nextchar);
	    optind++;
	    return '?';
	  }
	if (pfound != NULL)
	  {
	    option_index = indfound;
	    if (*nameend)
	      {
		/* Don't test has_arg with >, because some C compilers don't
		   allow it to be used on enums.  */
		if (pfound->has_arg)
		  optarg = nameend + 1;
		else
		  {
		    if (print_errors)
		      {
#if defined _LIBC && defined USE_IN_LIBIO
			char *buf;

			if (__asprintf (&buf, _("\
%s: option `-W %s' doesn't allow an argument\n"),
					argv[0], pfound->name) >= 0)
			  {
			    if (_IO_fwide (stderr, 0) > 0)
			      __fwprintf (stderr, L"%s", buf);
			    else
			      fputs (buf, stderr);

			    free (buf);
			  }
#else
			fprintf (stderr, _("\
%s: option `-W %s' doesn't allow an argument\n"),
				 argv[0], pfound->name);
#endif
		      }

		    nextchar += strlen (nextchar);
		    return '?';
		  }
	      }
	    else if (pfound->has_arg == 1)
	      {
		if (optind < argc)
		  optarg = argv[optind++];
		else
		  {
		    if (print_errors)
		      {
#if defined _LIBC && defined USE_IN_LIBIO
			char *buf;

			if (__asprintf (&buf, _("\
%s: option `%s' requires an argument\n"),
					argv[0], argv[optind - 1]) >= 0)
			  {
			    if (_IO_fwide (stderr, 0) > 0)
			      __fwprintf (stderr, L"%s", buf);
			    else
			      fputs (buf, stderr);

			    free (buf);
			  }
#else
			fprintf (stderr,
				 _("%s: option `%s' requires an argument\n"),
				 argv[0], argv[optind - 1]);
#endif
		      }
		    nextchar += strlen (nextchar);
		    return optstring[0] == ':' ? ':' : '?';
		  }
	      }
	    nextchar += strlen (nextchar);
	    if (longind != NULL)
	      *longind = option_index;
	    if (pfound->flag)
	      {
		*(pfound->flag) = pfound->val;
		return 0;
	      }
	    return pfound->val;
	  }
	  nextchar = NULL;
	  return 'W';	/* Let the application handle it.   */
      }
    if (temp[1] == ':')
      {
	if (temp[2] == ':')
	  {
	    /* This is an option that accepts an argument optionally.  */
	    if (*nextchar != '\0')
	      {
		optarg = nextchar;
		optind++;
	      }
	    else
	      optarg = NULL;
	    nextchar = NULL;
	  }
	else
	  {
	    /* This is an option that requires an argument.  */
	    if (*nextchar != '\0')
	      {
		optarg = nextchar;
		/* If we end this ARGV-element by taking the rest as an arg,
		   we must advance to the next element now.  */
		optind++;
	      }
	    else if (optind == argc)
	      {
		if (print_errors)
		  {
		    /* 1003.2 specifies the format of this message.  */
#if defined _LIBC && defined USE_IN_LIBIO
		    char *buf;

		    if (__asprintf (&buf, _("\
%s: option requires an argument -- %c\n"),
				    argv[0], c) >= 0)
		      {
			if (_IO_fwide (stderr, 0) > 0)
			  __fwprintf (stderr, L"%s", buf);
			else
			  fputs (buf, stderr);

			free (buf);
		      }
#else
		    fprintf (stderr,
			     _("%s: option requires an argument -- %c\n"),
			     argv[0], c);
#endif
		  }
		optopt = c;
		if (optstring[0] == ':')
		  c = ':';
		else
		  c = '?';
	      }
	    else
	      /* We already incremented `optind' once;
		 increment it again when taking next ARGV-elt as argument.  */
	      optarg = argv[optind++];
	    nextchar = NULL;
	  }
      }
    return c;
  }
}


int AFXAPI getopt_long (
     int argc,
     char *const *argv,
     const char *options,
     const struct option *long_options,
     int *opt_index)
{
  return _getopt_internal (argc, argv, options, long_options, opt_index, 0);
}







} // extern "C"
