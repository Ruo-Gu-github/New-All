#include "cuda_runtime.h"
#include "device_launch_parameters.h"

//cudaError_t InterpolationWithCUDA(float *host_vec_out, float *host_matrix_in, int *host_vec_in, float *host_vec_in_2, int size);
__global__ void MatrixKernel(short *c, float *a, int *b, float *d, short* e, int size, int slice, int slicesize, int pitch, int width, int height, int length, int min)
{
    int i = blockDim.x * blockIdx.x + threadIdx.x;
	float x, y, z;
	long long x0, x1, y0, y1, z0, z1;
	float deltaX0, deltaX1, deltaY0, deltaY1, deltaZ0, deltaZ1;
	float valueX0, valueX1, valueX2, valueX3, valueY0, valueY1, valueZ0;
	if (blockDim.x * blockIdx.x + threadIdx.x < size)
	{
		// a is a 4x4 matrix; b is a vector like [-528, -528].
		//c[i*3] = a[0] * b[i*2] + a[1] * b[i*2+1] + d[0];
		//c[i*3+1] = a[1] * b[i*2] + a[5] * b[i*2+1] + d[1];
		//c[i*3+2] = a[2] * b[i*2] + a[6] * b[i*2+1] + d[2];

		x = a[0] * (float)b[i*2] + a[4] * (float)b[i*2+1] + d[0];
		y = a[1] * (float)b[i*2] + a[5] * (float)b[i*2+1] + d[1];
		z = a[2] * (float)b[i*2] + a[6] * (float)b[i*2+1] + d[2];


		if (x < -width/2 || x >= width/2 - 1 || 
			y < -height/2 || y >= height/2 - 1 || 
			z < -length/2 || z >= length/2 - 1)
			c[i] = min;
		else
		{
			x += width/2;
			y += height/2;
			z += length/2;
			// cell 向上取整 floor 向下取整
			x0 = ceil(x);
			x1 = floor(x);
			y0 = ceil(y);
			y1 = floor(y);
			z0 = ceil(z);
			z1 = floor(z);

			deltaX0 = x0 - x;
			deltaX1 = 1.0 - deltaX0;
			deltaY0 = y0 - y;
			deltaY1 = 1.0 - deltaY0;
			deltaZ0 = z0 - z;
			deltaZ1 = 1.0 - deltaZ0;

			// pitch is imagesize counted by byte not short.

			valueX0 = e[z0 * pitch/2 + y0 * width + x0] * deltaX1 + e[z0 * pitch/2 + y0 * width + x1] * deltaX0;
			valueX1 = e[z0 * pitch/2 + y1 * width + x0] * deltaX1 + e[z0 * pitch/2 + y1 * width + x1] * deltaX0;
			valueX2 = e[z1 * pitch/2 + y0 * width + x0] * deltaX1 + e[z1 * pitch/2 + y0 * width + x1] * deltaX0;
			valueX3 = e[z1 * pitch/2 + y1 * width + x0] * deltaX1 + e[z1 * pitch/2 + y1 * width + x1] * deltaX0;

			valueY0 = valueX0 * deltaY1 + valueX1 * deltaY0;
			valueY1 = valueX2 * deltaY1 + valueX3 * deltaY0;

			valueZ0 = valueY0*deltaZ1 + valueY1*deltaZ0;

			c[i] = short(valueZ0);
		}
	}
}
// Helper function for using CUDA to transform vectors in parallel.
extern "C"
cudaError_t InterpolationWithCUDA(short *host_vec_out, float *host_matrix_in, float *host_vec_in, int *host_vec_in_2,
									short *dev_vec_out, float *dev_vec_in, short *dev_data_in, float *dev_matrix_in, int *dev_vec_in_2,
									int size, int slice, int sliceSize, int pitch_out, int width, int height, int length, int min)
{
  
	// use 16*16 thread per block
	const int threadsPerBlock = 256;
	const int blocksPerGrid =(size + threadsPerBlock - 1) / threadsPerBlock;

    cudaError_t cudaStatus;

	cudaStatus = cudaMemcpy(dev_matrix_in, host_matrix_in, sizeof(float) * 16, cudaMemcpyHostToDevice);
    if (cudaStatus != cudaSuccess) {
        goto Error;
    }

    // Copy input paramter from host memory to GPU buffers.
    cudaStatus = cudaMemcpy(dev_vec_in, host_vec_in, sizeof(float) * 3, cudaMemcpyHostToDevice);
    if (cudaStatus != cudaSuccess) {
        goto Error;
    }

    // Launch a kernel on the GPU with one thread for each element.
    MatrixKernel<<<blocksPerGrid, threadsPerBlock>>>(dev_vec_out, dev_matrix_in, dev_vec_in_2, dev_vec_in, dev_data_in, size, slice, sliceSize, pitch_out, width, height, length, min);

    // Check for any errors launching the kernel
    cudaStatus = cudaGetLastError();
    if (cudaStatus != cudaSuccess) {
        goto Error;
    }

    // cudaDeviceSynchronize waits for the kernel to finish, and returns
    // any errors encountered during the launch.
    cudaStatus = cudaDeviceSynchronize();
    if (cudaStatus != cudaSuccess) {
        goto Error;
    }

    // Copy output vector from GPU buffer to host memory.
    cudaStatus = cudaMemcpy(host_vec_out, dev_vec_out, size * sizeof(short), cudaMemcpyDeviceToHost);
    if (cudaStatus != cudaSuccess) {
        goto Error;
    }

	return cudaStatus;
Error:
    cudaFree(dev_vec_out);
    cudaFree(dev_vec_in);
	cudaFree(dev_vec_in_2);
    cudaFree(dev_matrix_in);
	cudaFree(dev_data_in);
    
    return cudaStatus;
}


// Helper function for using CUDA to transform vectors in parallel.
extern "C"
cudaError_t CopyDataToDevice(short *host_vec_out, float *host_vec_in, short** host_data_in, float *host_matrix_in, int *host_vec_in_2, 
							short *&dev_vec_out, float *&dev_vec_in, short *&dev_data_in, float *&dev_matrix_in, int *&dev_vec_in_2, 
							int imageSize, int slice, int sliceSize, int* pitch_out)
{
    cudaError_t cudaStatus;

	    // Choose which GPU to run on, change this on a multi-GPU system, default by device 0.
    cudaStatus = cudaSetDevice(0);
    if (cudaStatus != cudaSuccess) {
        goto Error;
    }

    // Allocate GPU buffers for vectors (3xN) and rotate matrix (4x4).
    cudaStatus = cudaMalloc((void**)&dev_vec_out, imageSize * sizeof(short));
    if (cudaStatus != cudaSuccess) {
        goto Error;
    }

    cudaStatus = cudaMalloc((void**)&dev_vec_in_2, imageSize * sizeof(int) * 2);
    if (cudaStatus != cudaSuccess) {
        goto Error;
    }

	cudaStatus = cudaMalloc((void**)&dev_vec_in, sizeof(float) * 3);
    if (cudaStatus != cudaSuccess) {
        goto Error;
    }

    cudaStatus = cudaMalloc((void**)&dev_matrix_in, sizeof(float) * 16);
    if (cudaStatus != cudaSuccess) {
        goto Error;
    }

	cudaStatus = cudaMallocPitch((void**)&dev_data_in, (size_t*)pitch_out, sliceSize * sizeof(short), slice);
    if (cudaStatus != cudaSuccess) {
        goto Error;
    }

    // Copy input paramter from host memory to GPU buffers.
    cudaStatus = cudaMemcpy(dev_vec_in_2, host_vec_in_2, imageSize * sizeof(int) * 2, cudaMemcpyHostToDevice);
    if (cudaStatus != cudaSuccess) {
        goto Error;
	}

	short* dev_data_in_head = dev_data_in;
	for (int i=0; i<slice; ++i)
	{
		cudaStatus = cudaMemcpy(dev_data_in_head, host_data_in[i], sliceSize * sizeof(short), cudaMemcpyHostToDevice);
		dev_data_in_head += *pitch_out / sizeof(short);
		if (cudaStatus != cudaSuccess) {
			goto Error;
		}
	}

	return cudaStatus;

Error:
    cudaFree(dev_vec_out);
    cudaFree(dev_vec_in);
	cudaFree(dev_vec_in_2);
    cudaFree(dev_matrix_in);
	cudaFree(dev_data_in);
    
    return cudaStatus;
}

// Helper function for using CUDA to transform vectors in parallel.
extern "C"
cudaError_t ReleaseDataFromDevice(short *dev_vec_out, float *dev_vec_in, short* dev_data_in, float *dev_matrix_in, int *dev_vec_in_2)
{
    cudaError_t cudaStatus;

    cudaFree(dev_vec_out);
    cudaFree(dev_vec_in);
	cudaFree(dev_vec_in_2);
    cudaFree(dev_matrix_in);
	cudaFree(dev_data_in);

	cudaStatus = cudaDeviceReset();
	if (cudaStatus != cudaSuccess) {
		return cudaStatus;
	}

    return cudaStatus;
}