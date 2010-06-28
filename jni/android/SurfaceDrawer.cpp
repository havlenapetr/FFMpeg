#include <ui/Surface.h>

static Surface *sSurface;

void setSurface(int *surface_ptr) {
	sSurface = (Surface *) surface_ptr;
}

void draw(const void * ptr, int size, int count) {

}
