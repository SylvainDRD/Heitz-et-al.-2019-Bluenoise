#include <mitsuba/render/sampler.h>

#include "mask.h"


MTS_NAMESPACE_BEGIN

class LDBNSampler : public Sampler {
public:
    LDBNSampler() : Sampler(Properties()) {}

    LDBNSampler(const Properties &props) : Sampler(props) { m_sampleCount = props.getSize("sampleCount", 4); }

    LDBNSampler(Stream *stream, InstanceManager *manager) : Sampler(stream, manager) {}

    virtual ~LDBNSampler() {}

    ref<Sampler> clone() override {
        ref<LDBNSampler> sampler = new LDBNSampler();
        sampler->m_sampleCount = m_sampleCount;
        sampler->m_sampleIndex = m_sampleIndex;

        return sampler.get();
    }

    void generate(const Point2i &offset) override {
        m_scrambleIndex = offset;
        m_sampleIndex = 0;
        m_dimension = 0;
    }

    void advance() {
        ++m_sampleIndex;
        m_dimension = 0;
    }

    Float next1D() override { return sample(m_scrambleIndex.y, m_scrambleIndex.x, m_sampleIndex, m_dimension++); }

    Point2 next2D() override {
        Point2 p;

        p.x = sample(m_scrambleIndex.y, m_scrambleIndex.x, m_sampleIndex, m_dimension++);
        p.y = sample(m_scrambleIndex.y, m_scrambleIndex.x, m_sampleIndex, m_dimension++);

        return p;
    }

    MTS_DECLARE_CLASS()
private:
    Point2i m_scrambleIndex;

    uint m_dimension;
};

MTS_IMPLEMENT_CLASS_S(LDBNSampler, false, Sampler)
MTS_EXPORT_PLUGIN(LDBNSampler, "Low discrepancy bluenoise sampler")
MTS_NAMESPACE_END
