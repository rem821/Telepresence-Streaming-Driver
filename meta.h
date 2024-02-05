//
// Created by standa on 2.2.24.
//
#pragma once

typedef struct {
    gboolean is_even;
    gint64 frame_id;
} FrameIdMeta;

#define GST_FRAME_ID_META_API_TYPE (GstFrameIdMetaApiGetType())

inline int64_t frameIdLeft = 0, frameIdRight = 0;

inline gboolean GstFrameIdMetaInit(GstMeta *meta, gpointer params, GstBuffer *buffer) {
    g_return_val_if_fail(meta != nullptr, FALSE);
    g_return_val_if_fail(buffer != nullptr, FALSE);

    // Ensure that the buffer is writable before modifying it
    if (!gst_buffer_is_writable(buffer)) {
        std::cout << "Buffer is not writable \n";
        return FALSE;
    }

    auto *frameIdMeta = (FrameIdMeta *) meta;
    frameIdMeta->frame_id = -1;
    frameIdMeta->is_even = false;
    return TRUE;
}

inline void GstFrameIdMetaFree(GstMeta *meta, GstBuffer *buffer) {
    auto *frameIdMeta = (FrameIdMeta *) meta;
    frameIdMeta->frame_id = 0;
    frameIdMeta->is_even = false;
}

inline guint GstFrameIdMetaApiGetType() {
    static GType type = 0;
    static const gchar *tags[] = {nullptr};

    if (g_once_init_enter(&type)) {
        std::cout << "Register API type \n";
        const GType _type = gst_meta_api_type_register("FrameIdMetaAPI", tags);
        g_once_init_leave(&type, _type);
    }

    return type;
}

inline const GstMetaInfo *GstFrameIdGetMetaInfo() {
    static const GstMetaInfo *frameIdMetaInfo = nullptr;

    if (g_once_init_enter(&frameIdMetaInfo)) {
        std::cout << "Register meta \n";
        const GstMetaInfo *meta = gst_meta_register(
            GST_FRAME_ID_META_API_TYPE,
            "FrameIdMeta",
            sizeof(FrameIdMeta),
            GstFrameIdMetaInit,
            GstFrameIdMetaFree,
            nullptr);
        g_once_init_leave(&frameIdMetaInfo, meta);
    }
    return frameIdMetaInfo;
}

inline FrameIdMeta *GstBufferAddFrameIdMeta(GstBuffer *buffer, int64_t frameId) {
    // check that gst_buffer valid
    g_return_val_if_fail(static_cast<int>(GST_IS_BUFFER(buffer)), nullptr);

    // check that gst_buffer writable
    if (!gst_buffer_is_writable(buffer))
        return nullptr;
    auto *meta = reinterpret_cast<FrameIdMeta *>(gst_buffer_add_meta(buffer, GstFrameIdGetMetaInfo(), nullptr));

    meta->is_even = !(frameId % 2);
    meta->frame_id = frameId;
    return meta;
}

inline int64_t GstBufferGetFrameIdMeta(GstBuffer *buffer) {
    const auto metadata = gst_buffer_get_meta(buffer, GST_FRAME_ID_META_API_TYPE);
    if (metadata == nullptr) {
        std::cout << "Meta not present down the line! \n";
        return -1;
    }
    const auto *frameIdMeta = reinterpret_cast<FrameIdMeta *>(metadata);
    return frameIdMeta->frame_id;
}

inline void OnIdentityHandoffMeta(const GstElement *identity, GstBuffer *buffer,  gpointer data) {
    const std::string pipelineName = identity->object.parent->name;

    if (pipelineName == "pipeline_camera_0") {
        GstBufferAddFrameIdMeta(buffer, frameIdLeft++);
    } else if (pipelineName == "pipeline_camera_1") {
        GstBufferAddFrameIdMeta(buffer, frameIdRight++);
    }

    gst_buffer_iterate_meta()
    const auto metadata = gst_buffer_get_meta(buffer, GST_FRAME_ID_META_API_TYPE);
    if (metadata == nullptr) {
        std::cout << "Meta not present! \n";
    } else {
        const auto *frameMeta = reinterpret_cast<FrameIdMeta *>(metadata);
        std::cout << "Meta present: " << std::to_string(frameMeta->frame_id) << " even: " << frameMeta->is_even << "! \n";
    }
}
