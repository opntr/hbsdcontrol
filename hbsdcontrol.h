#ifndef __HBSDCONTROL_H
#define	__HBSDCONTROL_H

enum pax_features_state {
	disable = 0,
	enable = 1
};

struct pax_feature_entry {
	const char	*feature;
	const char	*entry[2];
};

extern const struct pax_feature_entry pax_features[];

int hbsdcontrol_set_extattr(const char *file, const char *feature, const int val);
int hbsdcontrol_rm_extattr(const char *file, const char *feature);

#endif /* __HBSDCONTROL_H */
