/*
 * Copyright 2007-2010 National ICT Australia (NICTA), Australia
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


#include <log.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <libxml/tree.h>
//#include <libxml/parser.h>
#include "filter/factory.h"
#include "client.h"
#include "oml_value.h"

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

static OmlFilter*
parse_filter_properties(
    xmlNodePtr  el,
    OmlFilter*  f,
    OmlMStream* ms,
    OmlMP*      mp
);

static int
set_filter_property(
    OmlFilter*  f,
    const char* pname,
    const char* ptype,
    const char* pvalue
);

/**************************************************/

/**
 * \fn int parse_config(char* configFile)
 * \brief Parse the config file to configure the oml deaomon
 * \param configFile
 * \return 0 if successful <0 otherwise
 */
int
parse_config(
  char* configFile
) {
  xmlDocPtr doc;
  xmlNodePtr cur;

  if ((doc = xmlParseFile(configFile)) == NULL) {
    logerror("Config file '%s' not parsed successfully.\n",
      configFile);
    return -1;
  }

  if ((cur = xmlDocGetRootElement(doc)) == NULL) {
    logerror("Config file '%s' is empty.\n", configFile);
    xmlFreeDoc(doc);
    return -2;
  }

  if (xmlStrcmp(cur->name, (const xmlChar *)CONFIG_ROOT_NAME)) {
    logerror("Config file has wrong root, should be '%s'.\n",
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

/**
 * \fn static int parse_collector(xmlNodePtr el)
 * \brief Parse the definition of a single collector. A collector is effectively a writer plus an associated set of fiters for a set of measurement points.
 * \param el the element to analyse
 * \return 0 if successful <0 otherwise
 */
static int
parse_collector(
    xmlNodePtr el
) {
  xmlChar* url = xmlGetProp(el, (const xmlChar*)"url");
  if (url == NULL) {
    logerror("Missing 'url' attribute for '%s'.\n", el->name);
    return -1;
  }
  OmlWriter* writer;
  if ((writer = create_writer((char*)url)) == NULL) return -2;

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

/**
 * \fn static int parse_mp(xmlNodePtr el, OmlWriter* writer)
 * \brief Parse the collect information for a specific MP
 * \param el the element to analyse
 * \param writer the writer that will be use when creating the stream
 * \return 0 if successful <0 otherwise
 */
static int
parse_mp(
    xmlNodePtr el,
    OmlWriter* writer
) {
  xmlChar* name = xmlGetProp(el, (const xmlChar*)"name");
  if (name == NULL) {
    logerror("Missing 'name' attribute for '%s'.\n", el->name);
    return -1;
  }
  xmlChar* samplesS = xmlGetProp(el, (const xmlChar*)"samples");
  xmlChar* intervalS = xmlGetProp(el, (const xmlChar*)"interval");
  if (samplesS == NULL && intervalS == NULL) {
    logerror("Missing 'samples' or 'interval' attribute for '%s'.\n", el->name);
    return -2;
  }
  if (samplesS != NULL && intervalS != NULL) {
    logerror("Only one of 'samples' or 'interval' attribute can be defined for '%s'.\n",
        el->name);
    return -3;
  }
  OmlMP* mp = omlc_instance->mpoints;
  for (; mp != NULL; mp = mp->next) {
    if (strcmp((char*)name, mp->name) == 0) {
      break;
    }
  }
  if (mp == NULL) {
    logerror("Unknown measurement point '%s'.\n", name);
    return -4;
  }

  //OmlMStream* ms = (OmlMStream*)malloc(sizeof(OmlMStream));
  int samples = samplesS != NULL ? atoi((char*)samplesS) : -1;
  if (samples == 0) samples = 1;
  double interval = intervalS != NULL ? atof((char*)intervalS) : -1;
  OmlMStream* ms = create_mstream(interval, samples, mp, writer);

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
    create_default_filters(mp, ms);
  }
  ms->next = mp->firstStream;
  mp->firstStream = ms;
  if (interval > 0) {
    filter_engine_start(ms);
  }
  return 0;
}

/**
 * \fn static OmlFilter* parse_filter(xmlNodePtr  el, OmlMStream* ms, OmlMP* mp)
 * \brief Parse a filter def and return the configured filter.
 * \param el the xml node
 * \param ms the stream to associate with the filter
 * \param mp the measurement point structure
 * \return an OmlFilter if successful NULL otherwise
 */
static OmlFilter*
parse_filter(
    xmlNodePtr  el,
    OmlMStream* ms,
    OmlMP*      mp
) {
  OmlFilter* f = NULL;
  int index = -1;

  xmlChar* pname = xmlGetProp(el, (const xmlChar*)FILTER_PARAM_NAME_ATTR);
  if (pname == NULL) {
    // pname is optional, but issue warning if not declared 'multi_pnames'
    xmlChar* multi = xmlGetProp(el, (const xmlChar*)FILTER_MULTI_PARAM_ATTR);
    if (multi == NULL) {
      logwarn("No '%s' or '%s' found.\n",
        FILTER_PARAM_NAME_ATTR, FILTER_MULTI_PARAM_ATTR);
      return NULL;
    }
  } else {
    // find index
    OmlMPDef* dp = mp->param_defs;
    int i = 0;
    for (; dp->name != NULL; i++, dp++) {
      if (strcmp((char*)pname, dp->name) == 0) {
        if (i >= mp->param_count) {
          logerror("Index '%i' out of bounds.\n", i);
          return NULL;
        }
        index = i;
        break;
      }
    }
  }

  xmlChar* fname = xmlGetProp(el, (const xmlChar*)FILTER_NAME_ATTR);
  xmlChar* sname = xmlGetProp(el, (const xmlChar*)FILTER_STREAM_NAME_ATTR);
  OmlMPDef* def = (index < 0) ? NULL : &mp->param_defs[index];
  if (fname == NULL) {
    // pick default one
    if (def == NULL) {
      logerror("Can't create default filter without '%s' declaration.\n",
               FILTER_PARAM_NAME_ATTR);
      return NULL;
    }
    f = create_default_filter(def, ms, index);
  } else {
    if (def) {
      const char* name = (sname != NULL) ? (char*)sname : def->name;
      f = create_filter((const char*)fname, name, def->param_types, index);
    } else {
      if (sname == NULL) {
    logerror("Require '%s' attribute for multi_pname filter '%s'.\n",
          FILTER_STREAM_NAME_ATTR, fname);
    return NULL;
      }
      f = create_filter((const char*)fname, (const char*)sname, 0, -1);
    }
  }
  if (f != NULL) {
    f = parse_filter_properties(el, f, ms, mp);
  }
  return f;
}

/**
 * \fn static OmlFilter* parse_filter_properties(xmlNodePtr  el, OmlFiler* f, OmlMStream* ms, OmlMP* mp)
 * \brief Parse optional filter properties and call the filter's 'set' funtion with the properly cast values.
 * \param el the filer xml node
 * \param f filter
 * \param ms the stream to associate with the filter
 * \param mp the measurement point structure
 * \return an OmlFilter if successful NULL otherwise
 */
static OmlFilter*
parse_filter_properties(
    xmlNodePtr  el,
    OmlFilter*  f,
    OmlMStream* ms,
    OmlMP*      mp
) {
  (void)ms; // FIXME:  Are these parameters really not needed?
  (void)mp;
  xmlNodePtr el2 = el->children;
  for (; el2 != NULL; el2 = el2->next) {
    if (!xmlStrcmp(el2->name, (const xmlChar *)FILTER_PROPERTY_EL)) {
      if (f->set == NULL) {
        xmlChar* fname = xmlGetProp(el, (const xmlChar*)FILTER_NAME_ATTR);
        logerror("Filter '%s' doesn't support setting properties.\n",
                 fname);
        return NULL;
      }

      xmlChar* pname = xmlGetProp(el2, (const xmlChar*)FILTER_PROPERTY_NAME_ATTR);
      if (pname == NULL) {
        logerror("Can't find property name in filter ('%s') property declaration.\n",
                 f->name);
        return NULL;
      }
      xmlChar* ptype = xmlGetProp(el2, (const xmlChar*)FILTER_PROPERTY_TYPE_ATTR);
      if (ptype == NULL) ptype = (xmlChar*)"string";

      xmlNodePtr vel = el2->children;
      xmlChar* value = NULL;
      for (; vel != NULL; vel = vel->next) {
        if (vel->type == XML_TEXT_NODE) {
          value = vel->content;
          break;
        }
      }
      if (value == NULL) {
        logerror("Missing property ('%s') value in filter '%s'.\n",
                 pname, f->name);
        return NULL;
      }
      logdebug("Found filter property: %s:%s = '%s'.\n",
               pname, ptype, value);
      set_filter_property(f, (char*)pname, (char*)ptype, (char*)value);
    }
  }
  return f;
}

/**
 * \fn static int set_filter_property(OmlFilter* f, const char* pname, const char* ptype, const char* pvalue)

 * \brief Set property 'pname' on filter to 'pvalue' of type 'ptype'.
 * \param f filter
 * \param pname Name of property
 * \param ptype Type of property value
 * \param pvalue Value to set property to
 *
 * \return 1 on success, 0 or less for failure
 */
static int
set_filter_property(
    OmlFilter*  f,
    const char* pname,
    const char* ptype,
    const char* pvalue
) {
  OmlValue v;

  if (oml_value_from_s (&v, pvalue) == -1) {
    logerror("Error converting property '%s' value from string '%s'\n",
             pname, pvalue);
    return 0;
  }

  return f->set(f, pname, &v);
}


/**
 * \fn static char* getAttr(xmlNodePtr el, const char* attrName)
 * \brief give a char* representation of the value of an xml node
 * \param el the element the function will retrun the value
 * \param attrName the name of the node
 * \return a char* representing the value of the attribute
 */
static char*
getAttr(
    xmlNodePtr el,
    const char* attrName
) {
  xmlChar* attrVal;
  char* val = NULL;
  attrVal = xmlGetProp(el, (const xmlChar*)attrName);
  if (attrVal != NULL) {
    size_t len = strlen ((char*)attrVal) + 1;
    val = (char*)malloc(len * sizeof (char));
    memset (val, 0, len);
    strncpy(val, (char*)attrVal, len);
    xmlFree(attrVal);
  }

  return val;
}

/*
 Local Variables:
 mode: C
 tab-width: 4
 indent-tabs-mode: nil
 End:
*/
