#include "myLib.h"
#include "sds_lib.h"

void myFunc (unsigned int size, unsigned int dim, dataType_t threshold, dataType_t * data0, dataType_t * data1, dataType_t * data2)
{
	unsigned int i, k, l;

	for ( i = 0 ; i < size ; i ++ )
	{
		for ( k = 0 ; k < dim ; k ++ )
		{
			data2 [ i*dim + k ] = 0.0 ;
		}
		for ( k = 0 ; k < dim ; k ++ )
		{
			for ( l = 0 ; l < dim ; l ++ )
			{
				data2 [ i*dim + k ] += data0 [ k * dim + l ] * data1 [ i*dim+ l ];
			}			
		}
		
		int r = 1 ;
		for ( l = 0 ; r && ( l < dim ) ; l ++ )
		{
			r = ( data2 [ i*dim + l ] > threshold );
		}
		if ( r )
		{
			for ( l = 0 ; l < dim ; l ++ )
			{
				data2 [ i*dim + l ] = 0.0;
			}
		}
	}
}


int main(int argc, char ** argv)
{
	unsigned int i,j;

	// elenxos eisodwn
	assert(argc==5);

	unsigned int seed = (unsigned int)atoi(argv[1]);
	assert(seed>=0);

	srand(seed);

	unsigned int size = (unsigned int)atoi(argv[2]);
	assert(size>=1);

	unsigned int dim = (unsigned int)atoi(argv[3]);
	assert( dim==4 );

	dataType_t threshold = (dataType_t)atof(argv[4]);
	assert(threshold>=0.0);

	printf("\nSeed %u\nSize %u\nDimension %u\nThreshold %f\n", seed, size, dim, threshold);
	fflush(stdout);	

	dataType_t * data0 = (dataType_t *) sds_alloc(sizeof(dataType_t)*dim*dim);
	assert(data0!=NULL);

	// init data0
	for(i=0;i<dim*dim;i++)
	{
		dataType_t t = (float)(rand()%10);
		dataType_t d = ((float)(rand()%10))/10;
		data0[i] = t+d;
	}

	dataType_t * data1 = (dataType_t *) sds_alloc(sizeof(dataType_t)*dim*size);
	assert(data1!=NULL);

	// init data1
	for(i=0;i<dim*size;i++)
	{
		dataType_t t = (float)(rand()%10);
		dataType_t d = ((float)(rand()%10))/10;
		data1[i] = t+d;
	}


	/****************************************/
	/* Part 1: Reference Software Execution */
	/****************************************/

	dataType_t * data2_sw = (dataType_t *)malloc(sizeof(dataType_t)*dim*size);
	assert(data2_sw!=NULL);

	/* timing */
	double totalTime_sw=0.0;
	struct timespec timerStart_sw;
	struct timespec timerStop_sw;

	clock_gettime(CLOCK_REALTIME, &timerStart_sw);

	// klhsh reference
	myFunc(size, dim, threshold, data0, data1, data2_sw);

	clock_gettime(CLOCK_REALTIME, &timerStop_sw);
	totalTime_sw = (timerStop_sw.tv_sec-timerStart_sw.tv_sec)+ (timerStop_sw.tv_nsec-timerStart_sw.tv_nsec) / BILLION;

	printf("\nSoftware execution time: %f\n", totalTime_sw);
	fflush(stdout);


	/******************************/
	/* Part 2: Hardware Execution */
	/******************************/

	dataType_t * data2_hw = (dataType_t *) sds_alloc(sizeof(dataType_t)*dim*size);
	assert(data2_hw!=NULL);

	/* timing */
	double totalTime_hw=0.0;
	struct timespec timerStart_hw;
	struct timespec timerStop_hw;

	clock_gettime(CLOCK_REALTIME, &timerStart_hw);

	int loops = size/2;
	// asyncronh klhsh accelerators
	#pragma SDS async(1)
	#pragma SDS resource(1)
	myFuncAccel4( loops , dim , threshold , data0 , (dataType_bus*) data1 , (dataType_bus*) data2_hw );

	// ipologismos deiktwn kai ari8mo epanalipsewn gia ton deutero accelerator
	int offset = size % 2;

	loops = loops + offset;
	dataType_t * pointer1 = data1 + ( (size - offset) / 2 ) * dim;
	dataType_t * pointer2 = data2_hw + ( (size - offset) / 2 ) * dim;

	#pragma SDS async(2)
	#pragma SDS resource(2)
	myFuncAccel4( loops , dim , threshold , data0 , (dataType_bus*) pointer1 , (dataType_bus*) pointer2 );

	// wait tous accelerator
	#pragma SDS wait(1)
	#pragma SDS wait(2)


	clock_gettime(CLOCK_REALTIME, &timerStop_hw);
	totalTime_hw = (timerStop_hw.tv_sec-timerStart_hw.tv_sec)+ (timerStop_hw.tv_nsec-timerStart_hw.tv_nsec) / BILLION;

	printf("Hardware execution time: %f\n", totalTime_hw);
	fflush(stdout);


	/******************************/
	/* Elenxos or8othtas			*/
	/******************************/

	printf("\n");

	// compare outputs
	char string_hw[7], string_sw[7];
	int flag = 1;

	/*	Ta apotelesmata exoun mexri 2 xrhsima dekadika pshfia. Gia na apofigw false positive error kanw stroggulopoihsh.	*/
	for(i = 0; i < size; i++){
		for(j = 0; j < dim; j++){
			// 2 decimal places needed. Use sprintf for rounding
			sprintf(string_hw, "%.2f", data2_hw[i*dim + j]);
			sprintf(string_sw, "%.2f", data2_sw[i*dim + j]);
			// sprintf outputs strings. Need strcmp
			if(strcmp(string_hw, string_sw) != 0){
				printf("error pos %d: hw = %s, sw =  %s\n", i*dim + j, string_hw, string_sw);
				flag = 0;
			}
		}
	}
	if(flag)
		printf("no errors\n\n");

	// clear data
	sds_free(data0);
	sds_free(data1);
	free(data2_sw);
	sds_free(data2_hw);

	return 0;
}
