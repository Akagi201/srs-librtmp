/*
Author - Anshul Yadav
CopyRight - BirdDog
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
#include <cstring>

#include "srs_librtmp.hpp"

#include <Processing.NDI.Advanced.h>



int main(int argc, char **argv) {
    printf("publish ndi camera as rtmp stream to server like Nginx/FMLE/FFMPEG/Encoder\n");
    printf("BirdDog Ndi over Rtmp url publishing application .\n");

    if (argc <= 2) {
        printf("Usage: %s \"<ndi camera>\" rtmp://nginxServerIP:1935/bdlive/stream\n", argv[0]);
        printf("     ndi camera: H264 streaming NDI camera.e.g. \"P100-OHLEH (CAM_HX)\"\n");
        printf("     rtmp_publish_url: the rtmp publish url to server, vlc connect to this server.\n");
        printf("For example:\n");
        printf("     %s rtmp://192.168.208.52:1935/bdlive/pubapp\n", argv[0]);
        exit(-1);
    }

 //const char* cameraName = "PTZOpticsCamera (Channel 1)";
    char *cameraName = argv[1];
    const char *rtmp_url = argv[2];
    srs_human_trace("camera name =%s, rtmp_url=%s", cameraName, rtmp_url);

    // connect rtmp context
    srs_rtmp_t rtmp = srs_rtmp_create(rtmp_url);

    if (srs_rtmp_handshake(rtmp) != 0) {
        srs_human_trace("Rtmp handshake failed.");
        srs_rtmp_destroy(rtmp);
        return -1;
    }
    srs_human_trace("Rtmp handshake success");

    if (srs_rtmp_connect_app(rtmp) != 0) {
        srs_human_trace("connect vhost/app failed.");
        srs_rtmp_destroy(rtmp);
        return -1;
    }
    srs_human_trace("connect vhost/app success");

    if (srs_rtmp_publish_stream(rtmp) != 0) {
        srs_human_trace("publish stream failed.");
        srs_rtmp_destroy(rtmp);
        return -1;
    }
    srs_human_trace("publish stream success");

   if (!NDIlib_initialize())
        return -1;

    NDIlib_recv_create_v3_t mRecvType_H264;

    mRecvType_H264.p_ndi_recv_name = "RTMP-PUB-H264";
    mRecvType_H264.color_format = (NDIlib_recv_color_format_e)NDIlib_recv_color_format_ex_compressed_v5_with_audio;
    mRecvType_H264.bandwidth = NDIlib_recv_bandwidth_highest;

    NDIlib_source_t src;
    src.p_ndi_name = argv[1];
    //src.p_ip_address = "192.168.208.51";
    mRecvType_H264.source_to_connect_to = src;

    NDIlib_recv_instance_t pNDI_recv = NDIlib_recv_create_v4(&mRecvType_H264, 0);
    if (!pNDI_recv)
        return -1;

    NDIlib_recv_connect(pNDI_recv, &src);

    uint32_t dts = 0;
    uint32_t pts = 0;
    // @remark, the dts and pts if read from device, for instance, the encode lib,
    // so we assume the fps is 25, and each h264 frame is 1000ms/25fps=40ms/f.
    float fps = 25.0;
    uint32_t prevPts = 0;
    static uint32_t mFrameCounter = 0;
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
                    srs_rtmp_destroy(rtmp);
                   	// Destroy the receiver
                    NDIlib_recv_destroy(pNDI_recv);
                    NDIlib_destroy();
                    return -1;
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

    srs_human_trace("streaming sending exited.");
    srs_rtmp_destroy(rtmp);
    // Destroy the receiver
    NDIlib_recv_destroy(pNDI_recv);
    NDIlib_destroy();
    return 0;
}

