// clang-format off
#include "ADM_default.h"
#include "ADM_coreVideoFilterFunc.h"
#include "ADM_coreVideoFilter.h"
#include "ADM_videoFilterBridge.h"
// clang-format on

ADM_coreVideoFilter *bridge = NULL;

static int objectCount = 0;

/**
    \fn ADM_vf_clearFilters
*/
bool ADM_vf_clearFilters(void)
{
    ADM_info("clear filters\n");

    int nb = ADM_VideoFilters.size();

    for (int i = 0; i < nb; i++)
    {
        delete ADM_VideoFilters[i].instance;
    }

    ADM_VideoFilters.clear();
    // delete bridge also...
    if (bridge)
    {
        delete bridge;
        bridge = NULL;
    }

    return true;
}

/**
    \fn ADM_vf_getPluginFromTag
    \brief
*/
ADM_vf_plugin *ADM_vf_getPluginFromTag(uint32_t tag)
{
    for (int cat = 0; cat < VF_MAX; cat++)
    {
        int nb = ADM_videoFilterPluginsList[cat].size();

        for (int i = 0; i < nb; i++)
        {
            if (ADM_videoFilterPluginsList[cat][i]->tag == tag)
            {
                return ADM_videoFilterPluginsList[cat][i];
            }
        }
    }

    ADM_error("Cannot get video filter from tag %" PRIu32 "\n", tag);
    ADM_assert(0);

    return NULL;
}

/**
    \fn ADM_vf_duplicateFilterAtIndex

*/
bool ADM_vf_duplicateFilterAtIndex(IEditor *editor, int index)
{
    ADM_info("Duplicate video filter at index %d\n", index);

    ADM_assert(index < ADM_VideoFilters.size());

    ADM_VideoFilterElement *e = &(ADM_VideoFilters[index]);
    CONFcouple *conf = NULL;
    if (!e->instance->getCoupledConf(&conf))
    {
        ADM_warning("Cannot get configuration\n");
        return false;
    }

    ADM_coreVideoFilter *last = ADM_vf_getLastVideoFilter(editor);
    ADM_coreVideoFilter *nw = ADM_vf_createFromTag(e->tag, last, conf);

    if (!nw)
        return false;

    ADM_VideoFilterElement ne;
    ne.tag = e->tag;
    ne.enabled = e->enabled;
    ne.instance = nw;
    ne.objectId = objectCount++;
    ADM_VideoFilters.append(ne);

    return ADM_vf_recreateChain();
}
/**
    \fn ADM_vf_removeFilterAtIndex

*/
bool ADM_vf_removeFilterAtIndex(int index)
{
    ADM_info("Deleting video filter at index %d\n", index);

    ADM_assert(index < ADM_VideoFilters.size());
    // last filter, destroy..
    ADM_VideoFilterElement *e = &(ADM_VideoFilters[index]);

    delete e->instance;
    ADM_VideoFilters.removeAt(index);

    return ADM_vf_recreateChain();
}

/**
    \fn ADM_vf_toggleFilterEnabledAtIndex

*/
bool ADM_vf_toggleFilterEnabledAtIndex(int index)
{
    ADM_info("Toggle video filter enabled at index %d\n", index);

    ADM_assert(index < ADM_VideoFilters.size());

    ADM_VideoFilterElement *e = &(ADM_VideoFilters[index]);
    e->enabled = !e->enabled;

    return ADM_vf_recreateChain();
}
/**
    \fn ADM_vf_recreateChain
    \brief Rebuild the whole filterchain
*/
bool ADM_vf_recreateChain(void)
{
    ADM_assert(bridge);

    ADM_coreVideoFilter *f = bridge;
    BVector<ADM_coreVideoFilter *> bin;

    for (int i = 0; i < ADM_VideoFilters.size(); i++)
    {
        // Get configuration
        CONFcouple *c;
        ADM_coreVideoFilter *old = ADM_VideoFilters[i].instance;
        uint32_t tag = ADM_VideoFilters[i].tag;
        bool enabled = ADM_VideoFilters[i].enabled;

        old->getCoupledConf(&c);

        ADM_coreVideoFilter *nw = ADM_vf_createFromTag(tag, f, c);

        ADM_VideoFilters[i].instance = nw;
        ADM_VideoFilters[i].enabled = enabled;
        bin.append(old);

        if (c)
        {
            delete c;
        }

        if (enabled)
            f = nw;
    }

    // Now delete bin
    for (int i = 0; i < bin.size(); i++)
    {
        delete bin[i];
    }

    bin.clear();

    return true;
}

/**
    \fn ADM_vf_createFromTag
    \brief Create a new filter from its tag
*/
ADM_coreVideoFilter *ADM_vf_createFromTag(uint32_t tag, ADM_coreVideoFilter *last, CONFcouple *couples)
{
    ADM_vf_plugin *plugin = ADM_vf_getPluginFromTag(tag);

    return plugin->create(last, couples);
}

/**
        \fn ADM_vf_addFilterFromTag
        \brief Add a new video filter (identified by tag) at the end of the activate filter list
*/
ADM_VideoFilterElement *ADM_vf_addFilterFromTag(IEditor *editor, uint32_t tag, CONFcouple *c, bool configure)
{
    if (tag == VF_INVALID_FILTER)
        return NULL;

    ADM_info("Creating video filter using tag %" PRIu32 " \n", tag);
    // Fetch the descriptor...

    ADM_coreVideoFilter *last = ADM_vf_getLastVideoFilter(editor);
    ADM_coreVideoFilter *nw = ADM_vf_createFromTag(tag, last, c);

    if (!nw)
        return NULL;

    if (configure && nw->configure() == false)
    {
        delete nw;
        return NULL;
    }

    ADM_VideoFilterElement e;
    e.tag = tag;
    e.enabled = true;
    e.instance = nw;
    e.objectId = objectCount++;
    ADM_VideoFilters.append(e);

    return &ADM_VideoFilters[ADM_VideoFilters.size() - 1];
}

ADM_VideoFilterElement *ADM_vf_insertFilterFromTag(IEditor *editor, uint32_t tag, CONFcouple *c, int index)
{
    ADM_info("Creating video filter using tag %" PRIu32 " \n", tag);
    // Fetch the descriptor...

    ADM_coreVideoFilter *last = ADM_vf_getLastVideoFilter(editor);
    ADM_coreVideoFilter *nw = ADM_vf_createFromTag(tag, last, c);

    if (!nw)
        return NULL;

    ADM_VideoFilterElement e;

    e.tag = tag;
    e.enabled = true;
    e.instance = nw;
    e.objectId = objectCount++;

    ADM_VideoFilters.insert(index, e);

    ADM_vf_recreateChain();

    return &ADM_VideoFilters[index];
}

/**
    \fn getLastVideoFilter
*/
ADM_coreVideoFilter *ADM_vf_getLastVideoFilter(IEditor *editor)
{
    ADM_coreVideoFilter *last = NULL;

    if (!ADM_VideoFilters.size())
    {
        if (!bridge)
        {
            bridge = new ADM_videoFilterBridge(editor, 0, -1LL);
        }

        last = bridge;
    }
    else
    {
        last = bridge;
        for (int i = (ADM_VideoFilters.size() - 1); i >= 0; i--)
        {
            if (ADM_VideoFilters[i].enabled)
            {
                last = ADM_VideoFilters[i].instance;
                break;
            }
        }
    }

    return last;
}

/**
    \fn updateBridge
*/
void ADM_vf_updateBridge(IEditor *editor)
{
    if (!bridge)
        return;
    bridge->updateBridge(0, -1LL);
    ADM_vf_recreateChain();
}
