/***********************************************************************
IFFChunk - Class to read data files in IFF format from a variety of
stream data sources. The DataSourceParam template parameter has to
provide typed read<> methods for standard data types.
Copyright (c) 2008-2009 Oliver Kreylos
***********************************************************************/

#ifndef IFFCHUNK_INCLUDED
#define IFFCHUNK_INCLUDED

#include <string.h>
#include <string>

template <class DataSourceParam>
class IFFChunk // Class to represent chunks in an IFF file
	{
	/* Embedded classes: */
	public:
	typedef DataSourceParam DataSource;
	
	/* Elements: */
	private:
	IFFChunk* parent; // Pointer to the chunk's parent, or 0 for the root chunk
	DataSource& dataSource; // Stream source for the chunk
	char chunkId[5]; // ID of this chunk, with NUL termination
	size_t chunkSize; // Unpadded size of this chunk in bytes
	size_t numBytesLeft; // Number of bytes not yet read from this chunk
	
	/* Constructors and destructors: */
	public:
	IFFChunk(DataSource& sDataSource) // Reads the root chunk from the given data source
		:parent(0),
		 dataSource(sDataSource)
		{
		/* Read the chunk header: */
		dataSource.template read<char>(chunkId,4);
		chunkId[4]='\0';
		chunkSize=dataSource.template read<unsigned int>();
		numBytesLeft=chunkSize;
		}
	IFFChunk(IFFChunk& sParent,bool subChunk =false) // Reads a child chunk from the given parent; if flag is true, chunk size is only 2 bytes
		:parent(&sParent),
		 dataSource(parent->dataSource)
		{
		/* Read the chunk header: */
		dataSource.template read<char>(chunkId,4);
		chunkId[4]='\0';
		chunkSize=subChunk?dataSource.template read<unsigned short>():dataSource.template read<unsigned int>();
		numBytesLeft=chunkSize;
		parent->numBytesLeft-=4+(subChunk?sizeof(unsigned short):sizeof(unsigned int));
		}
	
	/* Methods: */
	const char* getChunkId(void) const // Returns the chunk's ID
		{
		return chunkId;
		}
	bool isChunk(const char* id) const // Returns true if the given ID matches the chunk's ID
		{
		return memcmp(chunkId,id,4)==0;
		}
	size_t getChunkSize(void) const // Returns the size of the chunk
		{
		return chunkSize;
		}
	size_t getRestSize(void) const // Returns the number of bytes left in the chunk
		{
		return numBytesLeft;
		}
	void closeChunk(void) // Finishes reading the chunk
		{
		/* Read the rest of the chunk's data: */
		if(numBytesLeft>0)
			{
			char* chunkBuffer=new char[numBytesLeft];
			dataSource.template read<char>(chunkBuffer,numBytesLeft);
			delete[] chunkBuffer;
			}
		
		/* Skip the padding byte: */
		if(chunkSize&0x1)
			dataSource.template read<char>();
		
		/* Update the parent chunk's unread data size: */
		if(parent!=0)
			parent->numBytesLeft-=(chunkSize+1)&~0x1;
		}
	
	/* Basic read methods: */
	template <class DataParam>
	DataParam read(void) // Reads single value
		{
		DataParam result=dataSource.template read<DataParam>();
		numBytesLeft-=sizeof(DataParam);
		return result;
		}
	template <class DataParam>
	size_t read(DataParam* data,size_t numItems) // Reads array of values
		{
		dataSource.template read<DataParam>(data,numItems);
		numBytesLeft-=numItems*sizeof(DataParam);
		return numItems;
		}
	
	/* Specialized read methods: */
	std::string readString(void) // Reads a null-terminated string with padding
		{
		std::string result;
		char c;
		while((c=dataSource.template read<char>())!='\0')
			result.push_back(c);
		if(!(result.length()&0x1))
			dataSource.template read<char>();
		numBytesLeft-=(result.length()+2)&~0x1;
		return result;
		}
	unsigned int readVarInt(void) // Reads a variable-length unsigned integer
		{
		unsigned char c=dataSource.template read<unsigned char>();
		if(c==(unsigned char)0xff)
			{
			unsigned int result=(unsigned int)(dataSource.template read<unsigned char>());
			for(int i=2;i<4;++i)
				result=(result<<8)|(unsigned int)(dataSource.template read<unsigned char>());
			numBytesLeft-=sizeof(unsigned int);
			return result;
			}
		else
			{
			unsigned int result=(unsigned int)(c);
			result=(result<<8)|(unsigned int)(dataSource.template read<unsigned char>());
			numBytesLeft-=sizeof(unsigned short);
			return result;
			}
		}
	};

#endif
