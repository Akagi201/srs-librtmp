
/**
gcc srs_publish.c ../../objs/lib/srs_librtmp.a -g -O0 -lstdc++ -o srs_publish
*/


#include <stdio.h>
#include <stdlib.h>
#ifndef _WIN32
#include <unistd.h>
#endif
#include "../../src/libs/srs_librtmp.hpp"


#include <Processing.NDI.Advanced.h>

#define BDPrint(msg) printf("\n BDRtmpPub : %s ",msg );


const char* cameraName = "PTZOpticsCamera (Channel 1)";
//const char* cameraName = "ANSHUL-PC (P100-OHLEH (CAM_HX))";

int main(int argc, char **argv) {
    printf("publish rtmp stream to server like FMLE/FFMPEG/Encoder\n");
    printf("srs(simple-rtmp-server) client librtmp library.\n");
    printf("version: %d.%d.%d\n", srs_version_major(), srs_version_minor(), srs_version_revision());

    if (argc <= 1) {
        printf("Usage: %s <rtmp_url>\n"
                       "   rtmp_url     RTMP stream url to publish\n"
                       "For example:\n"
                       "   %s rtmp://127.0.0.1:1935/live/livestream\n",
               argv[0], argv[0]);
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

    // warn it .
    // @see: https://github.com/winlinvip/simple-rtmp-server/issues/126
    srs_human_trace("\033[33m%s\033[0m",
                    "[warning] it's only a sample to use librtmp. "
                            "please never use it to publish and test forward/transcode/edge/HLS whatever. "
                            "you should refer to this tool to use the srs-librtmp to publish the real media stream."
                            "read about: https://github.com/winlinvip/simple-rtmp-server/issues/126");
    srs_human_trace("rtmp url: %s", argv[1]);
    srs_rtmp_t rtmp = srs_rtmp_create(argv[1]);

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

    u_int32_t timestamp = 0;
    u_int32_t mPts = 0, mDts = 0, mPreviousPts = 0;
    static u_int32_t mFrameCounter = 0;
    for (;;)
    {

        NDIlib_video_frame_v2_t video_frame;
        NDIlib_audio_frame_v3_t audio_frame;
        NDIlib_metadata_frame_t metadata_frame;

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
                g_b_SourceTypeIsHEVC = true;
            }
            else if (NDIlib_compressed_FourCC_type_H264 == VideoFrameCompressedPacket.fourCC)
            {
                g_b_SourceTypeIsHEVC = false;
            }
            else
                continue;
            uint8_t* dataPtr = video_frame.p_data + VideoFrameCompressedPacket.version;
            uint8_t* inBuffer = new uint8_t[DataSize];
            memcpy(inBuffer, dataPtr, DataSize);
            int size = DataSize;
            char* data = (char*)malloc(DataSize);

            memcpy(data, dataPtr, DataSize);
            char type = SRS_RTMP_TYPE_VIDEO;
            int err = 0;
            mPts = (1000 * (video_frame.frame_rate_D / video_frame.frame_rate_N)) * 90 * (++mFrameCounter);
            mDts = mPreviousPts;
            mPreviousPts = mPts;
            //if((err = srs_rtmp_write_packet(rtmp, type, timestamp, data, size)) != 0) {
            if ((err = srs_h264_write_raw_frames(rtmp, data, size, mDts, mPts)) != 0) {
                BDPrint(err);
                goto rtmp_destroy;
            }
            NDIlib_recv_free_video_v2(pNDI_recv, &video_frame);
            break
        }
        case NDIlib_frame_type_audio:
        {
            NDIlib_compressed_packet_t compressed_frame;
            size_t fs = sizeof(NDIlib_compressed_packet_t);
            memcpy(&compressed_frame, audio_frame.p_data, fs);
            if ((NDIlib_FourCC_audio_type_ex_e)compressed_frame.fourCC == NDIlib_FourCC_audio_type_ex_AAC)
            {
                //(ProcDataFn)(audio_frame.p_data + fs, compressed_frame.data_size, audio_frame.sample_rate, audio_frame.no_samples, audio_frame.no_channels, false);
            }
            else if ((NDIlib_FourCC_audio_type_ex_e)compressed_frame.fourCC == NDIlib_FourCC_audio_type_ex_OPUS)
            {
                BDPrint("OPUS audio frames not supported, only AAC.");
            }
            else
            {
                //BDPrint("Uncompressed audio data is not supported.");
            }

            NDIlib_recv_free_audio_v3(pNDI_recv, &audio_frame);
        }
        break;

        // Meta data
        case NDIlib_frame_type_metadata:
            BDPrint("Meta data received.");
            NDIlib_recv_free_metadata(pNDI_recv, &metadata_frame);
            break;

        case NDIlib_frame_type_error:
            BDPrint("NDIlib_frame_type_error");
            break;
        case NDIlib_frame_type_status_change:
            BDPrint("NDIlib_frame_type_status_change");
            break;
        case NDIlib_frame_type_none:
            BDPrint("NDIlib_frame_type_none");
            break;
        default:
            break;

        }
    }

rtmp_destroy:

    // Destroy the receiver
    NDIlib_recv_destroy(pNDI_recv);
    NDIlib_destroy();
    srs_rtmp_destroy(rtmp);

    return 0;
}
