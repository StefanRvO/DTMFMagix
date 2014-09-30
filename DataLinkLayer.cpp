#include "DataLinkLayer.h"
using namespace std;

DataLinkLayer::DataLinkLayer()
{
    grabberThread=thread(toneGrabber,this);
}

void DataLinkLayer::PlaySync() //Play the sync sequence
{
    for(int i=0;i<7;i++) //Times the sequenc is played. May need to be adjusted later.
    {
        for(int j=0;j<4;j++)
        {
            Player.PlayDTMF(SyncSequence[i],TONELENGHT);
            Player.PlayDTMF(' ',SILENTLENGHT);
        }
    }
    for(int i=0;i<4;i++)
    {
        Player.PlayDTMF(SyncEnd[i],TONELENGHT);
        Player.PlayDTMF(' ',SILENTLENGHT);
    }
}

void DataLinkLayer::GetSync()
{
    SyncData SData;
    char RecordedSequence[4];
    int SequenceCounter=0;
    char LastNote='!'; 
    while(!ArrayComp(RecordedSequence,SyncSequence,4,SequenceCounter)) //Loop until we get syncsequence in buffer
    {
        vector<float> in=Rec.GetAudioData(TONELENGHT,0);
        float freq1max=0;
        int freq1Index=0;
        float freq2max=0;
        int freq2Index=0;
        for(int k=0;k<4;k++)
        {
            float gMag=goertzel_mag(Freqarray1[k],SAMPLE_RATE,in);
            if (gMag>freq1max) {freq1max=gMag; freq1Index=k;}
        }
        
        for(int k=0;k<4;k++)
        {
            float gMag=goertzel_mag(Freqarray2[k],SAMPLE_RATE,in);
            if (gMag>freq2max) {freq2max=gMag; freq2Index=k;}
        }
        
        char Note=DTMFTones[freq1Index*4+freq2Index];
        
        if (Note==LastNote) 
        {
            if (freq1max+freq2max>SData.MaxMagnitude) 
            {
                gettimeofday(&SData.tv,NULL);
            }
            usleep(1000);
        }
        else
        {
            cout << (SData.tv.tv_sec*1000000+SData.tv.tv_usec)/1000 << endl;
            RecordedSequence[SequenceCounter]=Note;
            SequenceCounter=(SequenceCounter+1)%4;
            
            for(int j=0;j<4;j++) cout << RecordedSequence[j];
            cout << endl;
            LastNote=Note;
        }
    }
    //This is the synctime for the last tone in the samplesequence
    synctime=SData.tv.tv_sec*1000000+SData.tv.tv_usec+TONELENGHT*1000;
    //Wait for syncend signal
    samplesSinceSync=0;
    while(!ArrayComp(RecordedSequence,SyncEnd,4,SequenceCounter))
    {
        timeval tv;
        gettimeofday(&tv,NULL);
        while(synctime+(TONELENGHT+SILENTLENGHT)*1000*(samplesSinceSync+1)>=tv.tv_sec*1000000+tv.tv_usec)
        {
            usleep(1000);
            gettimeofday(&tv,NULL);
            }
        //cout << (tv.tv_sec*1000000+tv.tv_usec-synctime)/1000. << endl;
        samplesSinceSync++;
        auto in=Rec.GetAudioData(TONELENGHT,0);
        float max=0;
        int freq1Index;
        int freq2Index;
        for(int k=0;k<4;k++)
        {
            float gMag=goertzel_mag(Freqarray1[k],SAMPLE_RATE,in);
            if (gMag>max) {max=gMag; freq1Index=k;}
        }
        max=0;
        for(int k=0;k<4;k++)
        {
            float gMag=goertzel_mag(Freqarray2[k],SAMPLE_RATE,in);
            if (gMag>max) {max=gMag; freq2Index=k;}
        }
        char Note=DTMFTones[freq1Index*4+freq2Index];
        RecordedSequence[SequenceCounter]=Note;
        SequenceCounter=(SequenceCounter+1)%4;
           
        for(int j=0;j<4;j++) cout << RecordedSequence[j];
        cout << endl;
                
    }
    synctime=synctime+(TONELENGHT+SILENTLENGHT)*1000*(samplesSinceSync+1);
    //This is the expected time for next tone
    
}

bool DataLinkLayer::GetPackage()
{
    timeval tv;
    char RecordedSequence[4];
    int SequenceCounter=0;
    while(!ArrayComp(RecordedSequence,EndSequence,4,SequenceCounter)) //Loop until we get endsequence in buffer
    {
        gettimeofday(&tv,NULL);
        while(synctime+(TONELENGHT+SILENTLENGHT)*1000*(samplesSinceSync)>tv.tv_sec*1000000+tv.tv_usec)
        {
            usleep(1000);
            gettimeofday(&tv,NULL);
        }
        samplesSinceSync++;
          
        vector<float> RecordedData=Rec.GetAudioData(TONELENGHT,0);
        float max=0;
        int freq1Index;
        int freq2Index;
        for(int k=0;k<4;k++)
        {
            float gMag=goertzel_mag(Freqarray1[k],SAMPLE_RATE,RecordedData);
            if (gMag>max) {max=gMag; freq1Index=k;}
        }
        if (max<SILENTLIMIT) return false;
        max=0;
        for(int k=0;k<4;k++)
        {
            float gMag=goertzel_mag(Freqarray2[k],SAMPLE_RATE,RecordedData);
            if (gMag>max) {max=gMag; freq2Index=k;}
        }
        if (max<SILENTLIMIT) return false;
        char Tone=DTMFTones[freq1Index*4+freq2Index];
        for(int a=0;a<=freq1Index*4+freq2Index;a++) cout << Tone;
        cout << endl;
        
        RecData.RecArray[RecData.NextElementToRecord]=Tone;
        RecData.NextElementToRecord++;
        RecData.NextElementToRecord%=RecData.RecArray.size();
        RecordedSequence[SequenceCounter]=Tone;
        SequenceCounter=(SequenceCounter+1)%4;
    }
    return true;
}
bool DataLinkLayer::DataAvaliable()
{
    if (RecData.LastDataElement!=RecData.NextElementToRecord) return true;
    return false;
}
char DataLinkLayer::GetNextTone() //Check with DataAvaliable first! Else the behavior is undefined
{
    char RetChar=RecData.RecArray[RecData.LastDataElement++];
    RecData.LastDataElement%=RecData.RecArray.size();
    return RetChar;
}

void DataLinkLayer::PlayTones(string Tones)
{
    for(char Tone : Tones)
    {
        Player.PlayDTMF(Tone,TONELENGHT);
        Player.PlayDTMF(' ',SILENTLENGHT);
    }
}

void toneGrabber(DataLinkLayer *DaLLObj)
{
    while(1)
    {
        DaLLObj->GetSync();
        DaLLObj->samplesSinceSync=0;
        /*if ( */DaLLObj->GetPackage() /*) if (CRC()) DaLLObj->SendACK();
        else DaLLObj->SendNACK() */ ;
    }
}

template <typename Type>
bool ArrayComp(Type *Array1, Type *Array2,int size,int index)
{   //Compare two arrays. return true on match, else false.
    //If index is given, Array1 will be right-rotated by that amount.
    for(int i=0;i<size;i++)
    {
        if( Array1[(i+index)%size]!=Array2[i]) return false;
    }
    return true;
}