QImageIO Plugin to read and display ILBM files, including color cycle animations.

**Work in progress!**

* [ ] Parse ILBM files
  * [x] uncompressed
  * [x] compression 1
  * [x] compression 2
  * [x] <= 8 bit planes
  * [x] non-indexed images (> 8, <= 24 bit planes)
  * [x] HAM
  * [x] SHAM
  * [ ] PCHG (works for some images, buggy for others)
* [x] Display static indexed ILBM files
* [x] Display color cycle animations
* [x] Display non-indexed images
* [ ] Display masks/transparent color
