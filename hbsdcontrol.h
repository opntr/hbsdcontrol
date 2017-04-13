#ifndef __HBSDCONTROL_H
#define	__HBSDCONTROL_H

int hbsdcontrol_set_extattr(const char *file, const char *feature, const int val);
int hbsdcontrol_rm_extattr(const char *file, const char *feature);

#endif /* __HBSDCONTROL_H */
