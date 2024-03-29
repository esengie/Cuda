// MP 5 Scan
// Given a list (lst) of length n
// Output its prefix sum = {lst[0], lst[0] + lst[1], lst[0] + lst[1] + ... + lst[n-1]}
// Due Tuesday, January 22, 2013 at 11:59 p.m. PST

#include    <wb.h>

#define BLOCK_SIZE 512 //@@ You can change this

#define wbCheck(stmt) do {                                 \
        cudaError_t err = stmt;                            \
        if (err != cudaSuccess) {                          \
            wbLog(ERROR, "Failed to run stmt ", #stmt);    \
            return -1;                                     \
        }                                                  \
    } while(0)

__global__ void scan(float * input, float * output, int len, float* xy) {
    //@@ Modify the body of this function to complete the functionality of
    //@@ the scan on the device
    //@@ You may need multiple kernel calls; write your kernels before this
    //@@ function and call them from here
    __shared__ float scan_array[BLOCK_SIZE];
  	if (blockIdx.x * BLOCK_SIZE + threadIdx.x < len)
      scan_array[threadIdx.x] = input[blockIdx.x * BLOCK_SIZE + threadIdx.x];
    else 
      scan_array[threadIdx.x] = 0;
  	
  	__syncthreads();
  
  	int stride = 1;
  	while(stride<BLOCK_SIZE)
    {
      int index = (threadIdx.x + 1)*stride*2-1;
      if (index< BLOCK_SIZE)
        scan_array[index] += scan_array[index - stride];
      stride *=2;
      __syncthreads();
    }
  
  	for (int stride = BLOCK_SIZE / 2; stride > 0; stride /= 2)
    {
      __syncthreads();
        int index = (threadIdx.x + 1)*stride*2-1;
      if (index + stride < BLOCK_SIZE)
        scan_array[index + stride] += scan_array[index];
    }
  
    __syncthreads();
  	if (blockIdx.x * BLOCK_SIZE + threadIdx.x < len)
  //  {
  		output[blockIdx.x * BLOCK_SIZE + threadIdx.x] = scan_array[threadIdx.x];
  //		xy[blockIdx.x] = output[(blockIdx.x) * BLOCK_SIZE];
  //  }
  //	else 
  xy[blockIdx.x+1] = scan_array[BLOCK_SIZE - 1];
  	xy[0] = 0;
     __syncthreads();
      
}

__global__ void total(float * input, int len, float* xy) {
 
  if (blockIdx.x * BLOCK_SIZE + threadIdx.x < len)
  	input[(blockIdx.x) * BLOCK_SIZE + threadIdx.x] += xy[blockIdx.x];
  
}
int main(int argc, char ** argv) {
    wbArg_t args;
    float * hostInput; // The input 1D list
    float * hostOutput; // The output list
    float * deviceInput;
    float * deviceOutput;
  	float * device;
  	float * deviceO;
  	float * deviceOp;
    int numElements; // number of elements in the list

    args = wbArg_read(argc, argv);

    wbTime_start(Generic, "Importing data and creating memory on host");
    hostInput = (float *) wbImport(wbArg_getInputFile(args, 0), &numElements);
    hostOutput = (float*) malloc(numElements * sizeof(float));
    wbTime_stop(Generic, "Importing data and creating memory on host");

    wbLog(TRACE, "The number of input elements in the input is ", numElements);

    wbTime_start(GPU, "Allocating GPU memory.");
    wbCheck(cudaMalloc((void**)&deviceInput, numElements*sizeof(float)));
    wbCheck(cudaMalloc((void**)&deviceOutput, numElements*sizeof(float)));
  	wbCheck(cudaMalloc((void**)&device, ((numElements-1) / BLOCK_SIZE + 2) *sizeof(float)));
 	wbCheck(cudaMalloc((void**)&deviceO, ((numElements-1) / BLOCK_SIZE + 2) *sizeof(float)));
  	wbCheck(cudaMalloc((void**)&deviceOp, ((numElements-1) / BLOCK_SIZE + 2) *sizeof(float)));
    wbTime_stop(GPU, "Allocating GPU memory.");

    wbTime_start(GPU, "Clearing output memory.");
    wbCheck(cudaMemset(deviceOutput, 0, numElements*sizeof(float)));
  	wbCheck(cudaMemset(device, 0, ((numElements-1) / BLOCK_SIZE + 2)*sizeof(float)));
  wbCheck(cudaMemset(deviceO, 0, ((numElements-1) / BLOCK_SIZE + 2)*sizeof(float)));
  wbCheck(cudaMemset(deviceOp, 0, ((numElements-1) / BLOCK_SIZE + 2)*sizeof(float)));
    wbTime_stop(GPU, "Clearing output memory.");

    wbTime_start(GPU, "Copying input memory to the GPU.");
    wbCheck(cudaMemcpy(deviceInput, hostInput, numElements*sizeof(float), cudaMemcpyHostToDevice));
    wbTime_stop(GPU, "Copying input memory to the GPU.");

    //@@ Initialize the grid and block dimensions here
	dim3 dimGrid((numElements - 1)/BLOCK_SIZE + 1, 1, 1);
    dim3 dimBlock(BLOCK_SIZE,1,1);
  	dim3 dimGrid2((((numElements - 1)/BLOCK_SIZE+1)/BLOCK_SIZE) + 1, 1, 1);
    
    wbTime_start(Compute, "Performing CUDA computation");
    //@@ Modify this to complete the functionality of the scan
    //@@ on the deivce
	scan<<<dimGrid, dimBlock>>>(deviceInput, deviceOutput, numElements, device);
    cudaDeviceSynchronize();
  	scan<<<dimGrid2, dimBlock>>>(device, deviceO, ((numElements-1) / BLOCK_SIZE + 2), deviceOp);
    cudaDeviceSynchronize();
  	total<<<dimGrid, dimBlock>>>(deviceOutput, numElements, deviceO);
  	cudaDeviceSynchronize();  
  	wbTime_stop(Compute, "Performing CUDA computation");

    wbTime_start(Copy, "Copying output memory to the CPU");
    wbCheck(cudaMemcpy(hostOutput, deviceOutput, numElements*sizeof(float), cudaMemcpyDeviceToHost));
    wbTime_stop(Copy, "Copying output memory to the CPU");

    wbTime_start(GPU, "Freeing GPU Memory");
    cudaFree(deviceInput);
    cudaFree(deviceOutput);
  	cudaFree(device);
    cudaFree(deviceO);
    cudaFree(deviceOp);
    wbTime_stop(GPU, "Freeing GPU Memory");

    wbSolution(args, hostOutput, numElements);

    free(hostInput);
    free(hostOutput);

    return 0;
}
