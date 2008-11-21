/*
 * Copyright 2007-2008 National ICT Australia (NICTA), Australia
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */
/*!\file parse_config.c
  \brief Implements the parsing of the configuration file.
*/

#include <ocomm/o_log.h>
#include <malloc.h>
#include <string.h>
#include <libxml/tree.h>
//#include <libxml/parser.h>
#include "client.h"

static char*
getAttr(
    xmlNodePtr el,
    const char* attrName
);
static int
parse_collector(
    xmlNodePtr el
);
static int
parse_mp(
    xmlNodePtr el,
    OmlWriter* writer
);

static OmlFilter*
parse_filter(
    xmlNodePtr  el,
    OmlMStream* ms,
    OmlMP*      mp
);

/**************************************************/

int
parse_config(
  char* configFile
) {
  xmlDocPtr doc;
  xmlNodePtr cur;

  if ((doc = xmlParseFile(configFile)) == NULL) {
    o_log(O_LOG_ERROR, "Config file '%s' not parsed successfully.\n",
	  configFile);
    return -1;
  }

  if ((cur = xmlDocGetRootElement(doc)) == NULL) {
    o_log(O_LOG_ERROR, "Config file '%s' is empty.\n", configFile);
    xmlFreeDoc(doc);
    return -2;
  }

  if (xmlStrcmp(cur->name, (const xmlChar *)CONFIG_ROOT_NAME)) {
    o_log(O_LOG_ERROR, "Config file has wrong root, should be '%s'.\n",
	  CONFIG_ROOT_NAME);
    xmlFreeDoc(doc);
    return -3;
  }

  if (omlc_instance->node_name == NULL) {
    omlc_instance->node_name = getAttr(cur, NODE_ID_ATTR);
  }
  if (omlc_instance->experiment_id == NULL) {
    omlc_instance->experiment_id = getAttr(cur, EXP_ID_ATTR);
  }


  cur = cur->xmlChildrenNode;
  while (cur != NULL) {
    if (!xmlStrcmp(cur->name, (const xmlChar *)COLLECT_EL)) {
      if (parse_collector(cur)) {
        xmlFreeDoc(doc);
        return -4;
      }
    }
    cur = cur->next;
  }
  xmlFreeDoc(doc);
  return 0;
}

/* Parse the definition of a single collector. A collector
 * is effectively a writer plus an associated set of fiters
 * for a set of measurement points.
 */
static int
parse_collector(
    xmlNodePtr el
) {
  xmlChar* url = xmlGetProp(el, (const xmlChar*)"url");
  if (url == NULL) {
    o_log(O_LOG_ERROR, "Missing 'url' attribute for '%s'.\n", el->name);
    return -1;
  }
  OmlWriter* writer;
  if ((writer = create_writer(url)) == NULL) return -2;

  xmlNodePtr cur = el->xmlChildrenNode;
  while (cur != NULL) {
    if (!xmlStrcmp(cur->name, (const xmlChar *)MP_EL)) {
      if (parse_mp(cur, writer)) {
        return -3;
      }
    }
    cur = cur->next;
  }

  return 0;
}

/* Parse the collect information for a specific MP */
static int
parse_mp(
    xmlNodePtr el,
    OmlWriter* writer
) {
  xmlChar* name = xmlGetProp(el, (const xmlChar*)"name");
  if (name == NULL) {
    o_log(O_LOG_ERROR, "Missing 'name' attribute for '%s'.\n", el->name);
    return -1;
  }
  xmlChar* samplesS = xmlGetProp(el, (const xmlChar*)"samples");
  xmlChar* intervalS = xmlGetProp(el, (const xmlChar*)"interval");
  if (samplesS == NULL && intervalS == NULL) {
    o_log(O_LOG_ERROR, "Missing 'samples' or 'interval' attribute for '%s'.\n", el->name);
    return -2;
  }
  if (samplesS != NULL && intervalS != NULL) {
    o_log(O_LOG_ERROR, "Only one of 'samples' or 'interval' attribute can be defined for '%s'.\n",
        el->name);
    return -3;
  }
  OmlMP* mp = omlc_instance->mpoints;
  for (; mp != NULL; mp = mp->next) {
    if (strcmp(name, mp->name) == 0) {
      break;
    }
  }
  if (mp == NULL) {
    o_log(O_LOG_ERROR, "Unknown measurement point '%s'.\n", name);
    return -4;
  }

  //OmlMStream* ms = (OmlMStream*)malloc(sizeof(OmlMStream));
  int samples = samplesS != NULL ? atoi(samplesS) : -1;
  if (samples == 0) samples = 1;
  double interval = intervalS != NULL ? atof(intervalS) : -1;
  OmlMStream* ms = create_mstream(interval, samples, mp, writer);

  int i = 0;
  xmlNodePtr el2 = el->children;
  for (; el2 != NULL; el2 = el2->next) {
    if (!xmlStrcmp(el2->name, (const xmlChar *)FILTER_EL)) {
      OmlFilter* f = parse_filter(el2, ms, mp);
      if (f == NULL) {
        // too difficult to reclaim all memory
        return -5;
      }
      f->next = ms->firstFilter;
      ms->firstFilter = f;
    }
  }
  if (ms->firstFilter == NULL) {
    // no filters specified - use default
    createDefaultFilters(mp, ms);
  }
  ms->next = mp->firstStream;
  mp->firstStream = ms;
  if (interval > 0) {
    filter_engine_start(ms);
  }
  return 0;
}

/* Parse a filter def and return the configured filter. */
static OmlFilter*
parse_filter(
    xmlNodePtr  el,
    OmlMStream* ms,
    OmlMP*      mp
) {
  OmlFilter* f = NULL;
  xmlChar* indexS = xmlGetProp(el, (const xmlChar*)"i");
  if (indexS == NULL) {
    o_log(O_LOG_ERROR, "Missing 'i' attribute for '%s'.\n", el->name);
    return NULL;
  }
  int index = atoi(indexS);
  if (index >= mp->param_count || index < 0) {
    o_log(O_LOG_ERROR, "Index '%i' out of bounds.\n", index);
    return NULL;
  }

  xmlChar* name = xmlGetProp(el, (const xmlChar*)"name");
  OmlMPDef def = mp->param_defs[index];
  if (name == NULL) {
    // pick default one
    f = createDefaultFilter(&def, ms, index);
  } else {
    f = create_filter(name, def.name, def.param_types, index);
  }
  return f;
}

static char*
getAttr(
    xmlNodePtr el,
    const char* attrName
) {
  xmlChar* attrVal;
  char* val = NULL;
  attrVal = xmlGetProp(el, (const xmlChar*)attrName);
  if (attrVal != NULL) {
    val = (char*)malloc(sizeof(attrVal) + 1);
    strcpy(val, attrVal);
  }
  xmlFree(attrVal);
  return val;
}

/*
 Local Variables:
 mode: C
 tab-width: 4
 indent-tabs-mode: nil
*/
