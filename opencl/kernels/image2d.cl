__kernel void
image_test(__read_only image2d_t image1, __write_only image2d_t image2,
	const sampler_t sampler, int xi, int yi,
	__global float *xf, __global float *yf,
	__global int *result)
{
	write_imagef(image2, (int2)(yi++, xi++),
		read_imagef(image1, sampler, (int2)(xi++, yi++)));
	write_imagei(image2, (int2)(yi++, xi++),
		read_imagei(image1, sampler, (int2)(xi++, yi++)));
	write_imageui(image2, (int2)(yi++, xi++),
		read_imageui(image1, sampler, (int2)(xi++, yi++)));
	write_imageh(image2, (int2)(yi++, xi++),
		read_imageh(image1, sampler, (int2)(xi++, yi++)));

	write_imagef(image2, (int2)(yi++, xi++),
		read_imagef(image1, sampler, (float2)(xf[0], yf[0])));
	write_imagei(image2, (int2)(yi++, xi++),
		read_imagei(image1, sampler, (float2)(xf[1], yf[1])));
	write_imageui(image2, (int2)(yi++, xi++),
		read_imageui(image1, sampler, (float2)(xf[2], yf[2])));
	write_imageh(image2, (int2)(yi++, xi++),
		read_imageh(image1, sampler, (float2)(xf[3], yf[3])));

	result[0] = get_image_width(image1);
	result[1] = get_image_height(image1);
	//result[3] = get_image_depth(image1);
	result[4] = get_image_channel_data_type(image1);
	result[5] = get_image_channel_order(image1);
}

