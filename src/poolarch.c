/*
 * Copyright (c) 2007, Novell Inc.
 *
 * This program is licensed under the BSD license, read LICENSE.BSD
 * for further information
 */

/*
 * poolarch.c
 *
 * create architecture policies
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pool.h"
#include "poolid.h"
#include "poolarch.h"
#include "util.h"

static const char *archpolicies[] = {
#if defined(FEDORA) || defined(MAGEIA)
  "x86_64",	"x86_64:athlon:i686:i586:i486:i386",
#else
  "x86_64",	"x86_64:i686:i586:i486:i386",
#endif
  "i686",	"i686:i586:i486:i386",
  "i586",	"i586:i486:i386",
  "i486",	"i486:i386",
  "s390x",	"s390x:s390",
  "ppc64",	"ppc64:ppc",
  "ppc64p7",	"ppc64p7:ppc64:ppc",
  "ia64",	"ia64:i686:i586:i486:i386",
  "armv8hcnl",	"armv8hcnl:armv8hnl:armv8hl:armv7hnl:armv7hl:armv6hl",
  "armv8hnl",	"armv8hnl:armv8hl:armv7hnl:armv7hl:armv6hl",
  "armv8hl",	"armv8hl:armv7hl:armv6hl",
  "armv8l",	"armv8l:armv7l:armv6l:armv5tejl:armv5tel:armv5tl:armv5l:armv4tl:armv4l:armv3l",
  "armv7hnl",	"armv7hnl:armv7hl:armv6hl",
  "armv7hl",	"armv7hl:armv6hl",
  "armv7l",	"armv7l:armv6l:armv5tejl:armv5tel:armv5tl:armv5l:armv4tl:armv4l:armv3l",
  "armv6l",	"armv6l:armv5tejl:armv5tel:armv5tl:armv5l:armv4tl:armv4l:armv3l",
  "armv5tejl",	"armv5tejl:armv5tel:armv5tl:armv5l:armv4tl:armv4l:armv3l",
  "armv5tel",	"armv5tel:armv5tl:armv5l:armv4tl:armv4l:armv3l",
  "armv5tl",	"armv5tl:armv5l:armv4tl:armv4l:armv3l",
  "armv5l",	"armv5l:armv4tl:armv4l:armv3l",
  "armv4tl",	"armv4tl:armv4l:armv3l",
  "armv4l",	"armv4l:armv3l",
  "sh4a",	"sh4a:sh4",
  "sparc64v",	"sparc64v:sparc64:sparcv9v:sparcv9:sparcv8:sparc",
  "sparc64",	"sparc64:sparcv9:sparcv8:sparc",
  "sparcv9v",	"sparcv9v:sparcv9:sparcv8:sparc",
  "sparcv9",	"sparcv9:sparcv8:sparc",
  "sparcv8",	"sparcv8:sparc",
#if defined(FEDORA) || defined(MAGEIA)
  "ia32e",	"ia32e:x86_64:athlon:i686:i586:i486:i386",
  "athlon",	"athlon:i686:i586:i486:i386",
  "amd64",	"amd64:x86_64:athlon:i686:i586:i486:i386",
  "geode",	"geode:i586:i486:i386",
  "ppc64iseries", "ppc64iseries:ppc64:ppc",
  "ppc64pseries", "ppc64pseries:ppc64:ppc",
#endif
  "loongarch64", "loongarch64",
  0
};

void
pool_setarch(Pool *pool, const char *arch)
{
  if (arch)
    {
      int i;
      /* convert arch to known policy */
      for (i = 0; archpolicies[i]; i += 2)
	if (!strcmp(archpolicies[i], arch))
	  {
	    arch = archpolicies[i + 1];
	    break;
	  }
    }
  pool_setarchpolicy(pool, arch);
}

/*
 * we support three relations:
 *
 * a = b   both architectures a and b are treated as equivalent
 * a > b   a is considered a "better" architecture, the solver
 *         should change from a to b, but must not change from b to a
 * a : b   a is considered a "better" architecture, the solver
 *         must not change the architecture from a to b or b to a
 */
void
pool_setarchpolicy(Pool *pool, const char *arch)
{
  unsigned int score = 0x10001;
  size_t l;
  char d;
  Id *id2arch;
  Id id, lastarch;

  pool->id2arch = solv_free(pool->id2arch);
  pool->id2color = solv_free(pool->id2color);
  if (!arch)
    {
      pool->lastarch = 0;
      return;
    }
  id = pool->noarchid;
  lastarch = id + 255;
  /* note that we overallocate one element to be compatible with
   * old versions that accessed id2arch[lastarch].
   * id2arch[lastarch] will always be zero */
  id2arch = solv_calloc(lastarch + 1, sizeof(Id));
  id2arch[id] = 1;	/* the "noarch" class */

  d = 0;
  while (*arch)
    {
      l = strcspn(arch, ":=>");
      if (l)
	{
	  id = pool_strn2id(pool, arch, l, 1);
	  if (id >= lastarch)
	    {
	      id2arch = solv_realloc2(id2arch, (id + 255 + 1), sizeof(Id));
	      memset(id2arch + lastarch + 1, 0, (id + 255 - lastarch) * sizeof(Id));
	      lastarch = id + 255;
	    }
	  if (id2arch[id] == 0)
	    {
	      if (d == ':')
		score += 0x10000;
	      else if (d == '>')
		score += 0x00001;
	      id2arch[id] = score;
	    }
	}
      arch += l;
      if ((d = *arch++) == 0)
	break;
    }
  pool->id2arch = id2arch;
  pool->lastarch = lastarch;
}

unsigned char
pool_arch2color_slow(Pool *pool, Id arch)
{
  const char *s;
  unsigned char color;

  if ((unsigned int)arch >= (unsigned int)pool->lastarch)
    return ARCHCOLOR_ALL;
  if (!pool->id2color)
    pool->id2color = solv_calloc(pool->lastarch + 1, 1);
  s = pool_id2str(pool, arch);
  if (arch == ARCH_NOARCH || arch == ARCH_ALL || arch == ARCH_ANY)
    color = ARCHCOLOR_ALL;
  else if (!strcmp(s, "s390x") || strstr(s, "64"))
    color = ARCHCOLOR_64;
  else
    color = ARCHCOLOR_32;
  pool->id2color[arch] = color;
  return color;
}

