/*
 * Xpeccy+: the modules built - TrueType fonts, the light auto-hinter and the
 * gray/LCD renderer. Found before include/freetype/config/ftmodule.h because
 * thirdparty/freetype/xpeccy comes first on the include path.
 */

FT_USE_MODULE( FT_Module_Class, autofit_module_class )
FT_USE_MODULE( FT_Driver_ClassRec, tt_driver_class )
FT_USE_MODULE( FT_Module_Class, sfnt_module_class )
FT_USE_MODULE( FT_Renderer_Class, ft_smooth_renderer_class )

/* EOF */
