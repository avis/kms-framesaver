#include <config.h>
#include <gst/gst.h>

#include "gstkms_frame_saver.h"

static gboolean
init (GstPlugin * plugin)
{
  if (!gst_kms_frame_saver_plugin_init (plugin))
    return FALSE;

  return TRUE;
}

GST_PLUGIN_DEFINE (GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    kms_frame_saver,
    "Filter documentation",
    init, VERSION, GST_LICENSE_UNKNOWN, "PACKAGE_NAME", "origin")
