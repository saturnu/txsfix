#include <stdio.h>
#include <stdlib.h>
#include <libgen.h>

#include <string.h>
#include "./gopt.h"


#ifdef __unix__
//unix

#elif defined _WIN32 || defined _WIN64
//windows
    #include <Windows.h>
#else
#error "unknown platform"
#endif


const char *filename_input;
const char *filename_output;


#define MAJOR_VERSION 0
#define MINOR_VERSION 1
#define BUGFIX_VERSION 0

void *options;
int verbosity=0;



int main(int argc, const char **argv)
{



        options = gopt_sort( &argc, argv, gopt_start(
                                     gopt_option( 'h', 0, gopt_shorts( 'h' ), gopt_longs( "help" )),
                                     gopt_option( 'z', 0, gopt_shorts( 'z' ), gopt_longs( "version" )),

                                     gopt_option( 'v', GOPT_REPEAT, gopt_shorts( 'v' ), gopt_longs( "verbose" )),

                                     gopt_option( 'o', GOPT_ARG, gopt_shorts( 'o' ), gopt_longs( "output" )),
                                     gopt_option( 'i', GOPT_ARG, gopt_shorts( 'i' ), gopt_longs( "input" )) ));


        if( gopt( options, 'h' ) ) {


                fprintf( stdout, "Syntax: txsfix [options] -i input.txs -o output.txs\n\n");

                fprintf( stdout, "txsfix - a ngc winning eleven 6 txs fixer\n" );
                fprintf( stdout, "by saturnu <tt@anpa.nl>\n\n" );

                printf("Input:\n");
                fprintf( stdout, " -i, --input=input.txs\tinput txs file\n" );
                printf("Output:\n");
                fprintf( stdout, " -o, --output=output.txs\toutput txs file\n" );

                fprintf( stdout, " -v, --verbose\t\tverbose\n" );
                fprintf( stdout, " -z, --version\t\tversion info\n" );

                return 0;
        }


        if( gopt( options, 'z' ) ) {

                fprintf( stdout, "txsfix version v%d.%d.%d\n", MAJOR_VERSION, MINOR_VERSION, BUGFIX_VERSION );
                return 0;
        }



        if( !gopt( options, 'i' ) ) {
                //needed every time
                printf("input file missing! use 'txsfix -h' for help\n");
                return 0;
        }



        if(gopt( options, 'v' ) < 4)
                verbosity = gopt( options, 'v' );
        else verbosity = 3;

        if( verbosity > 1 )
                fprintf( stderr, "being really verbose\n" );

        else if( verbosity )
                fprintf( stderr, "being verbose\n" );




        if( gopt_arg( options, 'i', &filename_input ) && strcmp( filename_input, "-" ) ) {

                FILE *file;
                unsigned char *buffer;
                unsigned long fileLen;

                file = fopen(filename_input, "rb");


                if (!file)
                {
                        fprintf(stderr, "Unable to open file\n");
                        return 0;
                }

                //Get file length
                fseek(file, 0L, SEEK_END);
                fileLen=ftell(file);
                fseek(file, 0L, SEEK_SET);

                buffer=(unsigned char *)malloc(fileLen*sizeof(unsigned char));

                //  memset(buffer,0x00,fileLen+1);

                if (!buffer)
                {
                        fprintf(stderr, "Memory error!\n");
                        fclose(file);
                        return 0;
                }

                //Read file contents into buffer
                if (fread(buffer, sizeof(buffer[0]), fileLen, file) != fileLen) {
                        printf("read error\n");
                }


                fclose(file);



                /* offsets
                   0x4   4 byte filesize
                   0x20  1 byte texture counter
                   0x40  offset list a 4 byte times (tc) - offset=offset_entry+0x20
                 */


                int tc = buffer[0x20];

                if(verbosity>0)
                        printf("texture count: %d\n", tc);

                unsigned long filesize = buffer[0x4] << 24 | buffer[0x5] << 16 | buffer[0x6] << 8 | buffer[0x7];

                if(verbosity>0) {
                        printf("filesize header: %d\n", filesize);
                        printf("filesize real: %d\n", fileLen);
                }


                if(filesize!=fileLen) {

                        if(verbosity>0) {
                                printf("real filesize differs - fixing header\n");
                        }

                        buffer[0x4] = fileLen >> 24;
                        buffer[0x5] = fileLen >> 16;
                        buffer[0x6] = fileLen >> 8;
                        buffer[0x7] = fileLen;

                }



                unsigned long *offset_list;
                unsigned long *offset_list_real;
                offset_list=(unsigned long *)malloc(tc*sizeof(unsigned long));
                offset_list_real=(unsigned long *)malloc(tc*sizeof(unsigned long));

                memset(offset_list,0x00,tc*sizeof(unsigned long));
                memset(offset_list_real,0x00,tc*sizeof(unsigned long));

                if(verbosity>0)
                        printf("\nlist header offsets\n");

                int i=0;
                for (i=0; i<tc; i++) { //print header offset list

                        int offset = i*4 + 0x40;
                        offset_list[i] = buffer[0x0+offset] << 24 | buffer[0x1+offset] << 16 | buffer[0x2+offset] << 8 | buffer[0x3+offset];

                        if(verbosity>0)
                                printf("texture [%d] : %x\n", i+1, offset_list[i]+0x20);
                }

                //create real offset list
                if(verbosity>0)
                        printf("\nsearch for real offsets\n");

                unsigned long header_id=0x010301;
                int tc_real=0;
                for (i=0; i<fileLen; i++) {

                        if(memcmp(buffer+i, &header_id,4) == 0) {

                                if(verbosity>0)
                                        printf("found texture [%d] at: %x\n",tc_real+1,i);
                                offset_list_real[tc_real] = i; //store offsets
                                tc_real++;
                        }



                }

                //compare offset lists
                int injected_texture=0;
                char found=0;
                unsigned long buggy_toffset=999; //nothing found - offsets are identical

                for (i=0; i<tc; i++) {

                        if(  (offset_list[i]+0x20) != offset_list_real[i]) {
                                //offset differs, previous stream has a new length


                                int inject_offset = i*4 + 0x40;
                                buffer[inject_offset+0]=(offset_list_real[i]-0x20)>>24;
                                buffer[inject_offset+1]=(offset_list_real[i]-0x20)>>16;
                                buffer[inject_offset+2]=(offset_list_real[i]-0x20)>>8;
                                buffer[inject_offset+3]=(offset_list_real[i]-0x20); //overwrite wrong offsets

                                //mark false one
                                if(!found) {
                                        buggy_toffset=offset_list_real[i-1];
                                        injected_texture=i; //first texture has a fixed pos
                                        found=1;
                                }
                        }


                }


                if(buggy_toffset!=999) {

                        if(verbosity>0)
                                printf("\nfixing texture %d\n", injected_texture);

                        //Byte Swap Texture Header
                        unsigned char header_backup[0x20];
                        memset(header_backup, 0x00, 0x20*sizeof(unsigned char));


                        for (i=0; i<0x20; i++) {
                                header_backup[i]=buffer[buggy_toffset+i];
                        }


                        //compression size swap
                        buffer[buggy_toffset+4]=header_backup[7];
                        buffer[buggy_toffset+5]=header_backup[6];
                        buffer[buggy_toffset+6]=header_backup[5];
                        buffer[buggy_toffset+7]=header_backup[4];

                        buffer[buggy_toffset+8 ]=header_backup[11];
                        buffer[buggy_toffset+9 ]=header_backup[10];
                        buffer[buggy_toffset+10]=header_backup[9 ];
                        buffer[buggy_toffset+11]=header_backup[8 ];
                }


                if( gopt_arg( options, 'o', &filename_output ) && strcmp( filename_output, "-" ) ) {

                        FILE *write_ptr;
                        write_ptr = fopen(filename_output,"w+b"); // w for write, b for binary

                        if(fwrite(buffer,sizeof(buffer[0]),fileLen,write_ptr)==fileLen) {
                                printf("file written\n");
                        }else{
                                printf("write error!\n");
                        }


                        fclose(write_ptr);
                }else{
                        printf("\ndummy mode - use '-o filename.txs' to create a patched file\n");
                }


        }

        return 0;
}
