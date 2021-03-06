#include "Recorder.h"

using namespace std;
/* This routine will be called by the PortAudio engine when audio is needed.
It may called at interrupt level on some machines so don't do anything
that could mess up the system like calling malloc() or free().
*/
int Recorder::patestCallback(   const void *inputBuffer,void *outputBuffer,
                                unsigned long framesPerBuffer,
                                const PaStreamCallbackTimeInfo *timeInfo,
                                PaStreamCallbackFlags statusFlags,
                                void *userData )
{
    PortAudioData *data=(PortAudioData*) userData;
    //float *out =(float*) outputBuffer;
    float *in =(float*)inputBuffer;
    if (in==NULL) cout <<"ERROR Input buffer was 0!!" << endl;
    
    for(unsigned int i=0;i<framesPerBuffer;i++)
    {
        data->recBuff[data->NextRec++]=in[i]; //Maybe don't count NextRec up in the loop?? May cause problems
        if (data->NextRec>=SAMPLE_RATE*RECBUFLENGHT) data-> NextRec=0;
    }
    return paContinue;
}


Recorder::Recorder()
{
    PaError err;
    //Init PortAudio
    err = Pa_Initialize();
    if(err != paNoError) {cout << Pa_GetErrorText( err ) << endl; err = Pa_Terminate(); }
    //Create stream 1 input channel
    err=Pa_OpenDefaultStream (&stream, 
                                1, //input channels
                                0,
                                paFloat32,
                                SAMPLE_RATE,
                                10, //This may need to be changed (lowered, but this may introduce noise/artifacts. Testing is needed)
                                    //Setting to one apperently don't cause artefacts (need confirmation), but much greater CPU usage.
                                patestCallback,
                                &RecData );
    if(err != paNoError) {cout << Pa_GetErrorText( err ) << endl; err = Pa_Terminate(); }
    err = Pa_StartStream(stream);
    for(unsigned int i=0;i<RecData.recBuff.size();i++) RecData.recBuff[i]=0;
    cout << "Initialised audio recorder!" << endl;
    cout << "We use PortAudio version " << Pa_GetVersionText() << endl;
}


vector<float> Recorder::GetAudioData(int lenght ,int OffSet) /*OffSet= Get data from x milliseconds ago. Lenght=Lenght of recorded audio in ms*/ 
{
    //The buffer only have a certain lenght, don't request samples which has been thrown away!
    //Also, don't request future samples!
    if (OffSet+lenght>(RECBUFLENGHT*1000)*0.9 or OffSet<0) 
    {   
        cout << "Error getting Audio Data!!! Don't request Samples we don't have!!!" << endl;
        vector<float> rvec;
        return rvec;
    }
    //endpoint is the last sample to put in our array
    int endPoint=RecData.NextRec;
    endPoint -=(SAMPLE_RATE*OffSet)/1000;
    if (endPoint<0) endPoint+=SAMPLE_RATE*RECBUFLENGHT;
    //startPoint is the first sample to put in our array
    int startPoint=endPoint-(SAMPLE_RATE*lenght)/1000;
    if (startPoint<0) startPoint+=SAMPLE_RATE*RECBUFLENGHT;
    //Now, put the requsted samples into a array
    int i=startPoint;
    vector<float> Samples;
    Samples.reserve((SAMPLE_RATE*lenght)/1000);
    while(i!=endPoint)
    {
        //cout <<i << endl;
        Samples.push_back(RecData.recBuff[i]);
        i++;
        if (i==SAMPLE_RATE*RECBUFLENGHT) i=0;
    }
    return Samples;

}


Recorder::~Recorder()
{
    PaError err;
    err = Pa_CloseStream( stream );
    if(err != paNoError) {cout << Pa_GetErrorText( err ) << endl; err = Pa_Terminate(); }
    err = Pa_Terminate( );
    if(err != paNoError) {cout << Pa_GetErrorText( err ) << endl; err = Pa_Terminate(); }
}
