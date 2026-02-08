// Sample generator interface for BSDF sampling.

// Interface for random number generators used in BSDF sampling.
interface ISampleGenerator
{
    // Get next 1D uniform random sample in [0, 1)
    [mutating] float next1D();

    // Get next 2D uniform random sample in [0, 1)^2
    [mutating] float2 next2D();
};

// Helper function to get next 1D sample from generator
float sampleNext1D<S : ISampleGenerator>(inout S sg)
{
    return sg.next1D();
}

// Helper function to get next 2D sample from generator
float2 sampleNext2D<S : ISampleGenerator>(inout S sg)
{
    return sg.next2D();
}
