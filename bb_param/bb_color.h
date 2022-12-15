/* bb_color.h */

#ifndef BB_COLOR_H
#define BB_COLOR_H

#include "bb_param.h"
#include "core/object/object.h"

class BBColor : public BBParam {
	GDCLASS(BBColor, BBParam);

protected:
	virtual Variant::Type get_type() const override { return Variant::COLOR; }
};

#endif // BB_COLOR_H