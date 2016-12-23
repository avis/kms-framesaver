#ifndef _GST_KMS_FRAME_SAVER_H_
#define _GST_KMS_FRAME_SAVER_H_

#include <gst/video/gstvideofilter.h>

G_BEGIN_DECLS
#define GST_TYPE_KMS_FRAME_SAVER   (gst_kms_frame_saver_get_type())
#define GST_KMS_FRAME_SAVER(obj)   (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_KMS_FRAME_SAVER,Gstkms_frame_saver))
#define GST_KMS_FRAME_SAVER_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_KMS_FRAME_SAVER,Gstkms_frame_saverClass))
#define GST_IS_KMS_FRAME_SAVER(obj)   (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_KMS_FRAME_SAVER))
#define GST_IS_KMS_FRAME_SAVER_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_KMS_FRAME_SAVER))
typedef struct _Gstkms_frame_saver Gstkms_frame_saver;
typedef struct _Gstkms_frame_saverClass Gstkms_frame_saverClass;
typedef struct _Gstkms_frame_saverPrivate Gstkms_frame_saverPrivate;

struct _Gstkms_frame_saver
{
  GstVideoFilter base;
  Gstkms_frame_saverPrivate *priv;
};

struct _Gstkms_frame_saverClass
{
  GstVideoFilterClass base_kms_frame_saver_class;
};

GType gst_kms_frame_saver_get_type (void);

gboolean gst_kms_frame_saver_plugin_init (GstPlugin * plugin);

G_END_DECLS
#endif /* _GST_KMS_FRAME_SAVER_H_ */
