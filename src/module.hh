// module.hh

#ifndef AVRE_MODULE_HH
#define AVRE_MODULE_HH

class Module
{
public:
    Module();
    virtual ~Module();

    virtual void initialize() = 0;
    virtual void process() = 0;
};

#endif
