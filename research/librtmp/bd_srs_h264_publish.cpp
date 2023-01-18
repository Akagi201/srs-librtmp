/*
The MIT License (MIT)

Copyright (c) 2013-2015 winlin

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
the Software, and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/
/**
gcc srs_h264_raw_publish.c ../../objs/lib/srs_librtmp.a -g -O0 -lstdc++ -o srs_h264_raw_publish
*/

#include <stdio.h>
#include <stdlib.h>

#ifndef _WIN32
#include <unistd.h>
#endif
// for open h264 raw file.
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "../../src/libs/srs_librtmp.hpp"

#include <Processing.NDI.Advanced.h>

//const char* cameraName = "PTZOpticsCamera (Channel 1)";
const char* cameraName = "P100-OHLEH (CAM_HX)";


int main(int argc, char **argv) {
    printf("publish raw h.264 as rtmp stream to server like FMLE/FFMPEG/Encoder\n");
    printf("SRS(simple-rtmp-server) client librtmp library.\n");
    printf("version: %d.%d.%d\n", srs_version_major(), srs_version_minor(), srs_version_revision());

    if (argc <= 2) {
        printf("Usage: %s <h264_raw_file> <rtmp_publish_url>\n", argv[0]);
        printf("     h264_raw_file: the h264 raw steam file.\n");
        printf("     rtmp_publish_url: the rtmp publish url.\n");
        printf("For example:\n");
        printf("     %s ./720p.h264.raw rtmp://127.0.0.1:1935/live/livestream\n", argv[0]);
        printf("Where the file: http://winlinvip.github.io/srs.release/3rdparty/720p.h264.raw\n");
        printf("See: https://github.com/winlinvip/simple-rtmp-server/issues/66\n");
        exit(-1);
    }

    if (!NDIlib_initialize())
        return 0;

    NDIlib_recv_create_v3_t mRecvType_H264;

    mRecvType_H264.p_ndi_recv_name = "RTMP-PUB-H264";
    mRecvType_H264.color_format = (NDIlib_recv_color_format_e)NDIlib_recv_color_format_ex_compressed_v5_with_audio;//NDIlib_recv_color_format_ex_compressed_v3_with_audio;
    mRecvType_H264.bandwidth = NDIlib_recv_bandwidth_highest;

    NDIlib_source_t src;
    src.p_ndi_name = cameraName;
    src.p_ip_address = "192.168.208.51";
    mRecvType_H264.source_to_connect_to = src;

    NDIlib_recv_instance_t pNDI_recv = NDIlib_recv_create_v4(&mRecvType_H264, 0);
    if (!pNDI_recv)
        return 0;

    NDIlib_recv_connect(pNDI_recv, &src);

    const char *raw_file = argv[1];
    const char *rtmp_url = argv[2];
    srs_human_trace("raw_file=%s, rtmp_url=%s", raw_file, rtmp_url);

    // connect rtmp context
    srs_rtmp_t rtmp = srs_rtmp_create(rtmp_url);

    if (srs_rtmp_handshake(rtmp) != 0) {
        srs_human_trace("simple handshake failed.");
        goto rtmp_destroy;
    }
    srs_human_trace("simple handshake success");

    if (srs_rtmp_connect_app(rtmp) != 0) {
        srs_human_trace("connect vhost/app failed.");
        goto rtmp_destroy;
    }
    srs_human_trace("connect vhost/app success");

    if (srs_rtmp_publish_stream(rtmp) != 0) {
        srs_human_trace("publish stream failed.");
        goto rtmp_destroy;
    }
    srs_human_trace("publish stream success");

    u_int32_t dts = 0;
    u_int32_t pts = 0;
    // @remark, the dts and pts if read from device, for instance, the encode lib,
    // so we assume the fps is 25, and each h264 frame is 1000ms/25fps=40ms/f.
    float fps = 25.0;
    u_int32_t prevPts = 0;
    static u_int32_t mFrameCounter = 0;
    for (;;)
    {

        NDIlib_video_frame_v2_t video_frame;
        auto retType = NDIlib_recv_capture_v3(pNDI_recv, &video_frame, NULL, NULL, 1000);
        switch (retType)
        {
        case NDIlib_frame_type_video:
        {
            NDIlib_compressed_packet_t VideoFrameCompressedPacket;
            memcpy(&VideoFrameCompressedPacket, video_frame.p_data, sizeof(NDIlib_compressed_packet_t));
            int DataSize = VideoFrameCompressedPacket.data_size + VideoFrameCompressedPacket.extra_data_size;
            if (NDIlib_compressed_FourCC_type_HEVC == VideoFrameCompressedPacket.fourCC)
            {
                //g_b_SourceTypeIsHEVC = true;
            }
            else if (NDIlib_compressed_FourCC_type_H264 == VideoFrameCompressedPacket.fourCC)
            {
                //g_b_SourceTypeIsHEVC = false;
            }
            else
                continue;
            uint8_t* dataPtr = video_frame.p_data + VideoFrameCompressedPacket.version;
            int size = DataSize;
            char* data = (char*)malloc(DataSize);

            memcpy(data, dataPtr, DataSize);
            char type = SRS_RTMP_TYPE_VIDEO;
            fps = (float)((float)video_frame.frame_rate_N / (float)video_frame.frame_rate_D);
            pts += (video_frame.frame_rate_D*1000) / video_frame.frame_rate_N;
            dts = prevPts;
            prevPts = pts;
            
            // send out the h264 packet over RTMP
            int ret = srs_h264_write_raw_frames(rtmp, data, size, pts, pts);
            if (ret != 0) {
                if (srs_h264_is_dvbsp_error(ret)) {
                    srs_human_trace("ignore drop video error, code=%d", ret);
                }
                else if (srs_h264_is_duplicated_sps_error(ret)) {
                    srs_human_trace("ignore duplicated sps, code=%d", ret);
                }
                else if (srs_h264_is_duplicated_pps_error(ret)) {
                    srs_human_trace("ignore duplicated pps, code=%d", ret);
                }
                else {
                    srs_human_trace("send h264 raw data failed. ret=%d", ret);
                    goto rtmp_destroy;
                }
            }

            // 5bits, 7.3.1 NAL unit syntax, 
            // H.264-AVC-ISO_IEC_14496-10.pdf, page 44.
            u_int8_t nut = (char)data[0] & 0x1f;
            srs_human_trace("sent packet: type=%s, time=%d, size=%d, fps=%f, b[%d]=%#x(%s)",
                srs_human_flv_tag_type2string(SRS_RTMP_TYPE_VIDEO), dts, size, fps, 0,
                (char)data[0],
                (nut == 7 ? "SPS" : (nut == 8 ? "PPS" : (nut == 5 ? "I" : (nut == 1 ? "P" : "Unknown")))));

            NDIlib_recv_free_video_v2(pNDI_recv, &video_frame);
            break;
        }

        default:
        {
        }
        }
    }

    srs_human_trace("h264 raw data completed");

    rtmp_destroy:
    srs_rtmp_destroy(rtmp);
    return 0;
}

