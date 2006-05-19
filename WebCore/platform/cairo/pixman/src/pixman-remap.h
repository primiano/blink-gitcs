#define pixman_add_trapezoids _moz_cairo_pixman_add_trapezoids
#define pixman_color_to_pixel _moz_cairo_pixman_color_to_pixel
#define composeFunctions _moz_cairo_pixman_compose_functions
#define fbComposeSetupMMX _moz_cairo_pixman_compose_setup_mmx
#define pixman_composite _moz_cairo_pixman_composite
#define fbCompositeCopyAreammx _moz_cairo_pixman_composite_copy_area_mmx
#define fbCompositeSolidMask_nx8888x0565Cmmx _moz_cairo_pixman_composite_solid_mask_nx8888x0565Cmmx
#define fbCompositeSolidMask_nx8888x8888Cmmx _moz_cairo_pixman_composite_solid_mask_nx8888x8888Cmmx
#define fbCompositeSolidMask_nx8x0565mmx _moz_cairo_pixman_composite_solid_mask_nx8x0565mmx
#define fbCompositeSolidMask_nx8x8888mmx _moz_cairo_pixman_composite_solid_mask_nx8x8888mmx
#define fbCompositeSolidMaskSrc_nx8x8888mmx _moz_cairo_pixman_composite_solid_mask_src_nx8x8888mmx
#define fbCompositeSolid_nx0565mmx _moz_cairo_pixman_composite_solid_nx0565mmx
#define fbCompositeSolid_nx8888mmx _moz_cairo_pixman_composite_solid_nx8888mmx
#define fbCompositeSrc_8888RevNPx0565mmx _moz_cairo_pixman_composite_src_8888RevNPx0565mmx
#define fbCompositeSrc_8888RevNPx8888mmx _moz_cairo_pixman_composite_src_8888RevNPx8888_mmx
#define fbCompositeSrc_8888x8888mmx _moz_cairo_pixman_composite_src_8888x8888mmx
#define fbCompositeSrc_8888x8x8888mmx _moz_cairo_pixman_composite_src_8888x8x8888mmx
#define fbCompositeSrcAdd_8000x8000mmx _moz_cairo_pixman_composite_src_add_8000x8000mmx
#define fbCompositeSrcAdd_8888x8888mmx _moz_cairo_pixman_composite_src_add_8888x8888mmx
#define fbCompositeSrc_x888x8x8888mmx _moz_cairo_pixman_composite_src_x888x8x8888mmx
#define pixman_composite_trapezoids _moz_cairo_pixman_composite_trapezoids
#define pixman_composite_tri_fan _moz_cairo_pixman_composite_tri_fan
#define pixman_composite_tri_strip _moz_cairo_pixman_composite_tri_strip
#define pixman_composite_triangles _moz_cairo_pixman_composite_triangles
#define fbCopyAreammx _moz_cairo_pixman_copy_area_mmx
#define pixman_fill_rectangle _moz_cairo_pixman_fill_rectangle
#define pixman_fill_rectangles _moz_cairo_pixman_fill_rectangles
#define pixman_format_create _moz_cairo_pixman_format_create
#define pixman_format_create_masks _moz_cairo_pixman_format_create_masks
#define pixman_format_destroy _moz_cairo_pixman_format_destroy
#define pixman_format_get_masks _moz_cairo_pixman_format_get_masks
#define pixman_format_init _moz_cairo_pixman_format_init
#if defined(USE_MMX) && !defined(__amd64__) && !defined(__x86_64__)
#define fbHaveMMX _moz_cairo_pixman_have_mmx
#endif
#define pixman_image_create _moz_cairo_pixman_image_create
#define pixman_image_create_for_data _moz_cairo_pixman_image_create_for_data
#define pixman_image_destroy _moz_cairo_pixman_image_destroy
#define pixman_image_get_data _moz_cairo_pixman_image_get_data
#define pixman_image_get_depth _moz_cairo_pixman_image_get_depth
#define pixman_image_get_format _moz_cairo_pixman_image_get_format
#define pixman_image_get_height _moz_cairo_pixman_image_get_height
#define pixman_image_get_stride _moz_cairo_pixman_image_get_stride
#define pixman_image_get_width _moz_cairo_pixman_image_get_width
#define pixman_image_set_clip_region _moz_cairo_pixman_image_set_clip_region
#define pixman_image_set_component_alpha _moz_cairo_pixman_image_set_component_alpha
#define pixman_image_set_filter _moz_cairo_pixman_image_set_filter
#define pixman_image_set_repeat _moz_cairo_pixman_image_set_repeat
#define pixman_image_set_transform _moz_cairo_pixman_image_set_transform
#define pixman_image_create_linear_gradient _moz_cairo_pixman_image_create_linear_gradient
#define pixman_image_create_radial_gradient _moz_cairo_pixman_image_create_radial_gradient
#define miIsSolidAlpha _moz_cairo_pixman_is_solid_alpha
#define pixman_pixel_to_color _moz_cairo_pixman_pixel_to_color
#define pixman_region_append _moz_cairo_pixman_region_append
#define pixman_region_contains_point _moz_cairo_pixman_region_contains_point
#define pixman_region_contains_rectangle _moz_cairo_pixman_region_contains_rectangle
#define pixman_region_copy _moz_cairo_pixman_region_copy
#define pixman_region_create _moz_cairo_pixman_region_create
#define pixman_region_create_simple _moz_cairo_pixman_region_create_simple
#define pixman_region_destroy _moz_cairo_pixman_region_destroy
#define pixman_region_empty _moz_cairo_pixman_region_empty
#define pixman_region_extents _moz_cairo_pixman_region_extents
#define pixman_region_intersect _moz_cairo_pixman_region_intersect
#define pixman_region_inverse _moz_cairo_pixman_region_inverse
#define pixman_region_not_empty _moz_cairo_pixman_region_not_empty
#define pixman_region_num_rects _moz_cairo_pixman_region_num_rects
#define pixman_region_rects _moz_cairo_pixman_region_rects
#define pixman_region_reset _moz_cairo_pixman_region_reset
#define pixman_region_subtract _moz_cairo_pixman_region_subtract
#define pixman_region_translate _moz_cairo_pixman_region_translate
#define pixman_region_union _moz_cairo_pixman_region_union
#define pixman_region_union_rect _moz_cairo_pixman_region_union_rect
#define pixman_region_validate _moz_cairo_pixman_region_validate
#define RenderEdgeInit _moz_cairo_pixman_render_edge_init
#define RenderEdgeStep _moz_cairo_pixman_render_edge_step
#define RenderLineFixedEdgeInit _moz_cairo_pixman_render_line_fixed_edge_init
#define RenderSampleCeilY _moz_cairo_pixman_render_sample_ceil_y
#define RenderSampleFloorY _moz_cairo_pixman_render_sample_floor_y
#define fbSolidFillmmx _moz_cairo_pixman_solid_fill_mmx
