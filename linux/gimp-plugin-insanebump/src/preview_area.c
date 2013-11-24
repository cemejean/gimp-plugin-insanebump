/* GIMP Plug-in InsaneBump
 * Copyright (C) 2013  Derby Russsell <jdrussell51@gmail.com> (the "Author").
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHOR BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Except as contained in this notice, the name of the Author of the
 * Software shall not be used in advertising or otherwise to promote the
 * sale, use or other dealings in this Software without prior written
 * authorization from the Author.
 */
#include "config.h"

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "main.h"
#include "interface.h"
#include "scale.h"
#include "InsaneBump.h"

#include "plugin-intl.h"
#include "PluginConnectors.h"

/**
 * See /home/derby/Documents/InsaneBumpTutorialVideoWatched.txt
 * This explains all the other names of Previews that need to be added.
 */

/*  Constants  */


/*  Local function prototypes  */
void removeAllLayersExceptMain(void);


/*  Global Variables  */
int _active = 0;
GtkWidget *btn_d = NULL;

/*  Local variables  */
static GtkWidget *btn_ao = NULL;
static GtkWidget *btn_s = NULL;
static GtkWidget *btn_n = NULL;
//static GtkWidget *btn_h = NULL;
//static GtkWidget *btn_ln = NULL;
//static GtkWidget *btn_mn = NULL;
//static GtkWidget *btn_hn = NULL;
//static GtkWidget *btn_sn = NULL;
static gint32 drawableAOPrev_ID = -1;
static gint32 drawableDiffusePrev_ID = -1;
static gint32 drawableSpecularPrev_ID = -1;
static gint32 drawableNormalPrev_ID = -1;
static gint32 drawableBeginActiveLayer = -1;


/*  Private functions  */
static void draw_preview_area_update(GimpDrawable *drawable) {
    if(!is_3D_preview_active())
    {
        /** Adding all references to preview for second release. */
        if ((local_vals.image_ID != 0) && (drawable != NULL))
        {
            update_preview = 0;
            if (drawable->bpp == 3) {
                GimpPixelRgn amap_rgn;
                gint rowbytes = PREVIEW_SIZE * 3;
                gint nbytes = drawable->width * drawable->height * 3;
                guchar *tmp = g_new(guchar, nbytes);
                guchar dst[PREVIEW_RGB_SIZE];

                gimp_pixel_rgn_init(&amap_rgn, drawable, 0, 0, drawable->width, drawable->height, 0, 0);
                gimp_pixel_rgn_get_rect(&amap_rgn, tmp, 0, 0, drawable->width, drawable->height);

                scale_pixels(dst, PREVIEW_SIZE, PREVIEW_SIZE, tmp, drawable->width, drawable->height, drawable->bpp);

                gimp_preview_area_draw(GIMP_PREVIEW_AREA(preview), 0, 0, PREVIEW_SIZE, PREVIEW_SIZE,
                                       GIMP_RGB_IMAGE, dst, rowbytes);
                gtk_widget_queue_draw (preview);
                g_free (tmp);
                _active = 1;
            } else if (drawable->bpp == 4) {
                GimpPixelRgn amap_rgn;
                gint rowbytes = PREVIEW_SIZE * 4;
                gint nbytes = drawable->width * drawable->height * 4;
                guchar *tmp = g_new(guchar, nbytes);
                guchar dst[PREVIEW_RGBA_SIZE];

                gimp_pixel_rgn_init(&amap_rgn, drawable, 0, 0, drawable->width, drawable->height, 0, 0);
                gimp_pixel_rgn_get_rect(&amap_rgn, tmp, 0, 0, drawable->width, drawable->height);

                scale_pixels(dst, PREVIEW_SIZE, PREVIEW_SIZE, tmp, drawable->width, drawable->height, drawable->bpp);

                gimp_preview_area_draw(GIMP_PREVIEW_AREA(preview), 0, 0, PREVIEW_SIZE, PREVIEW_SIZE,
                                     GIMP_RGBA_IMAGE, dst, rowbytes);
                
                gtk_widget_queue_draw (preview);
                g_free (tmp);
                _active = 1;
            }
        }
    }
}

static GimpDrawable *preview_diffuse_only(gint32 image_ID, gint32 draw) {
    gint32 noiselayer_ID = -1;
    gint32 drawable_ID = -1;
    drawableBeginActiveLayer = gimp_image_get_active_layer(image_ID);
    drawable_ID = gimp_layer_copy (drawableBeginActiveLayer);
    gimp_image_add_layer(image_ID, drawable_ID, -1);
    gimp_image_set_active_layer(image_ID, drawable_ID);
    /** Here I should hide previous active layer, make not visible. */

    /**
     * For preview do nothing here. 
     *   if(local_vals.Resizie)
     *   {
     *   }
     */
    
    if (local_vals.Noise)
    {
        /** Already have active layer in drawable_ID. */
        // drawable_ID = gimp_image_get_active_layer(image_ID);
        noiselayer_ID = gimp_layer_copy (drawable_ID);

        gimp_image_add_layer(image_ID, noiselayer_ID, -1);
        gimp_image_set_active_layer(image_ID, noiselayer_ID);

        /**
         * Filter "RGB Noise" applied
         * Standard plug-in. Source code ships with GIMP.
         * 
         * Add the "f" here to signify float in c language.
         */
        if (plug_in_rgb_noise_connector(image_ID, noiselayer_ID, 1, 1, 0.20f, 0.20f, 0.20f, 0.0f) != 1) return 0;

        gimp_layer_set_mode(noiselayer_ID, GIMP_VALUE_MODE);
        gimp_image_merge_down(image_ID, noiselayer_ID, 0);
    }
         
    if(local_vals.RemoveLighting)
    {
        removeShadingPreview(image_ID, local_vals.Noise);
        /**
         * See notes inside of removeShadingPreview
         * for explanation of next line.
         */
        
        /**
         * If Noise is on, then noiselayer_ID was merged down into drawable_ID.
         * You cannot remove drawable_ID in this case, as it is the only 
         * layer left!
         */
        // if (!local_vals.Noise) {
        //     removeGivenLayerFromImage(image_ID, drawable_ID);
        // }
        
        //gint *pnLayers = NULL;
        //gint numLayers = 0 ;
        //pnLayers = gimp_image_get_layers(image_ID, &numLayers);
        //GString *gsTemp = g_string_new("Number of Layers ");
        //g_string_printf(gsTemp, "Number of Layers = %d pnLayers[0] = %d", numLayers, pnLayers[0]);
        //gimp_message(gsTemp->str);
        //g_string_free(gsTemp, TRUE);
        //// Said: "Number of Layers = 3 pnLayers[0] = 11"
        //// Closed InsaneBump and observed Background Copy #1 and Background layer names.
        
        /**
         * However, if Noise is Yes and RemoveLighting is Yes,
         * Then there is an extra layer floating around!
         * Delete noiselayer_ID?  I thought it was merged!
         * No, I was right noiselayer_ID is already gone!
         */
    }
		
    drawableDiffusePrev_ID = gimp_image_get_active_layer(image_ID);
    gimp_levels_stretch(drawableDiffusePrev_ID);
    if(local_vals.Tile)
    {
        /**
         * Filter "Tile Seamless" applied
         * Standard plug-in. Source code ships with GIMP.
         */
        if (plug_in_make_seamless_connector(image_ID, drawableDiffusePrev_ID) != 1) return 0;
    }
    
    /** Here I should un hide previously hidden layer, make visible. */
    
    if (draw == TRUE) {
        return gimp_drawable_get(drawableDiffusePrev_ID);
    }
    return NULL;
}

static void preview_alt_normal(gint32 image_ID) {
/*******************************************************************************
 * Begin H and Normal
 ******************************************************************************/    
    gint32 mergedLayer_ID = -1;
    gint32 normalmap_ID = -1;
    gfloat wsize = (gfloat)gimp_image_width(image_ID);
    gfloat hsize = (gfloat)gimp_image_width(image_ID);
    /** Get active layer. */
    gint32 drawable_temp_ID = gimp_image_get_active_layer(image_ID);
    /** Copy active layer. */
    /** LLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLL */
    gint32 diffuse_ID = gimp_layer_copy (drawable_temp_ID);
    /** Add new layer to image. */
    gimp_image_add_layer(image_ID, diffuse_ID, -1);
    /** Set new layer as active. */
    gimp_image_set_active_layer(image_ID, diffuse_ID);
    /** Here I should hide previous active layer, make not visible. */
    
    /** Since copied, don't need this here. */
    // gint32 diffuse_ID = gimp_image_get_active_layer(image_ID);
    /** blur seems to not create an extra layer. */
    blur(image_ID, diffuse_ID, wsize, hsize, local_vals.LargeDetails, 0);

    normalmap_ID = gimp_image_get_active_layer(image_ID);
    if(local_vals.smoothstep)
    {
        /**
         * Filter "Blur" applied
         * Standard plug-in. Source code ships with GIMP.
         */
        if (plug_in_blur_connector(image_ID, normalmap_ID) != 1) return;
    }

    if(local_vals.invh)
    {
        /**
         * Colors Menu->Invert
         * Standard plug-in. Source code ships with GIMP.
         */
        if (plug_in_vinvert_connector(image_ID, normalmap_ID) != 1) return;
    }

    /** Extra layer here. */
    /** LLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLL */
    doBaseMap(image_ID, diffuse_ID, local_vals.Depth, local_vals.LargeDetails);
    normalmap_ID = gimp_image_get_active_layer(image_ID);

    /** Creates an extra layer. */
    /** LLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLL */
    shapeRecognise(image_ID, normalmap_ID, local_vals.ShapeRecog);
    if(local_vals.smoothstep)
    {
        normalmap_ID = gimp_image_get_active_layer(image_ID);

        /**
         * Filter "Blur" applied
         * Standard plug-in. Source code ships with GIMP.
         */
        if (plug_in_blur_connector(image_ID, normalmap_ID) != 1) return;
    }

    /** Creates an extra layer. */
    /** LLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLL */
    sharpen(image_ID, diffuse_ID, local_vals.Depth, 0, local_vals.SmallDetails);
    normalmap_ID = gimp_image_get_active_layer(image_ID);

    /**
    * Filter Enhance "Sharpen" applied
    * Standard plug-in. Source code ships with GIMP.
    */
    if (plug_in_sharpen_connector(image_ID, normalmap_ID, 20) != 1) return;

    /** Creates an extra layer. */
    /** LLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLL */
    sharpen(image_ID, diffuse_ID, local_vals.Depth, 6, local_vals.MediumDetails);
    normalmap_ID = gimp_image_get_active_layer(image_ID);
    
    /**
     * Filter "Blur" applied
     * Standard plug-in. Source code ships with GIMP.
     */
    if (plug_in_blur_connector(image_ID, normalmap_ID) != 1) return;

    gimp_drawable_set_visible(diffuse_ID, 0);
    
    /** Don't do the next line: */
    // gimp_image_merge_visible_layers(image_ID, 0);
    /** Do this instead for preview: */
    mergedLayer_ID = gimp_layer_new_from_visible(image_ID, image_ID, "temp");
    /** Add copied layer to image. */
    gimp_image_add_layer(image_ID, mergedLayer_ID, -1);
    // removeGivenLayerFromImage(image_ID, newlayer_ID);
    /** Here I should un hide previously hidden layer, make visible. */

    // drawable_ID = gimp_image_get_active_layer(image_ID);
}

static GimpDrawable *preview_normal_only(gint32 image_ID, gint32 draw) {
    /** Do not do this for preview. */
    // gint32 diffuse_ID = gimp_image_get_active_layer(image_ID);
    // gimp_drawable_set_visible(diffuse_ID, 0);
    // gimp_image_merge_visible_layers(image_ID, 0);
    /**
     * The theory here is to leave the original layer untouched, 
     * copy the active original layer, make the copy active, 
     * then modify the active layer.  Show this layer to the 
     * preview slot, then delete the active layer, Leaving
     * the original drawing untouched for previews.
     */

    // gint32 drawable_temp_ID = -1;
    
    if (preview_diffuse_only(image_ID, FALSE) != NULL) return NULL;
    preview_alt_normal(image_ID);
    
    /** because we just ran preview_alt_normal, don't need to copy, just grab. */
//    /** Get active layer. */
//    drawable_temp_ID = gimp_image_get_active_layer(image_ID);
//    /** Copy active layer. */
//    drawableNormalPrev_ID = gimp_layer_copy (drawable_temp_ID);
//    /** Add layer to image. */
//    gimp_image_add_layer(image_ID, drawableNormalPrev_ID, -1);
//    /** Set the copied layer to be the active layer. */
//    gimp_image_set_active_layer(image_ID, drawableNormalPrev_ID);
//    /** Here I should hide previous active layer, make not visible. */

    /** Just grab. */
    drawableNormalPrev_ID = gimp_image_get_active_layer(image_ID);

    /** We just copied the active layer and made it active, don't need to do it again. */
    // drawableNormalPrev_ID = gimp_image_get_active_layer(image_ID);

    /**
     * Filter "Normalmap" applied
     * Non-Standard plug-in. Source code included.
     * 
     * original values:
     * plug_in_normalmap_derby(image, drawable, 0, 0.0, 1.0, 0, 0, 0, 8, 0, 0, 0, 0, 0.0, drawable)
     */
    nmapvals.filter = 0;
    nmapvals.minz = 0.0f;
    nmapvals.scale = 1.0f;
    nmapvals.wrap = 0;
    nmapvals.height_source = 0;
    nmapvals.alpha = 0;
    nmapvals.conversion = 8;
    nmapvals.dudv = 0;
    nmapvals.xinvert = 0;
    nmapvals.yinvert = 0;
    nmapvals.swapRGB = 0;
    nmapvals.contrast = 0.0f;
    nmapvals.alphamap_id = drawableNormalPrev_ID;
    normalmap(drawableNormalPrev_ID, 0);

    // gimp_file_save(GIMP_RUN_NONINTERACTIVE, image_ID, drawable_ID, file_name_temp->str, file_name_temp->str);

    /** Not needed because we are calling preview_alt_normal instead. */
    // removeGivenLayerFromImage(image_ID, drawable_temp_ID);
    /** Here I should un hide previously hidden layer, make visible. */
    
    if (draw == TRUE) {
        return gimp_drawable_get(drawableNormalPrev_ID);
    }
    return NULL;
}

static GimpDrawable *preview_ambient_occlusion_only(gint32 image_ID, gint32 draw) {
    // preview_diffuse_only(image_ID);
    if (preview_normal_only(image_ID, FALSE) != NULL) return NULL;
    drawableAOPrev_ID = gimp_image_get_active_layer(image_ID);

    /**
     * Colors ->  Components -> "Channel Mixer" applied
     * Standard plug-in. Source code ships with GIMP.
     * 
     * Add the "f" here to signify float in c language.
     * Removed 0.0 on first param and changed to 0 because boolean.
     */
    if (plug_in_colors_channel_mixer_connector(image_ID, drawableAOPrev_ID, 0, -200.0f, 0.0f, 0.0f, 0.0f, -200.0f, 0.0f, 0.0f, 0.0f, 1.0f) != 1) return 0;

    gimp_desaturate(drawableAOPrev_ID);
    gimp_levels_stretch(drawableAOPrev_ID);
    
    // removeGivenLayerFromImage(image_ID, drawableDiffusePrev_ID);
    // removeGivenLayerFromImage(image_ID, drawableNormalPrev_ID);
    
    if (draw == TRUE) {
        return gimp_drawable_get(drawableAOPrev_ID);
    }
    return NULL;
}

static GimpDrawable *preview_specular_only(gint32 image_ID) {
    gint32 nResult = 0;
    // gint32 temp_ID = -1;

    // preview_diffuse_only(image_ID);
    // preview_normal_only(image_ID);
    if (preview_ambient_occlusion_only(image_ID, FALSE) != NULL) return NULL;
    
    // temp_ID = gimp_image_get_active_layer(image_ID);

    if(local_vals.EdgeSpecular)
    {
        drawableSpecularPrev_ID = specularEdgeWorker(image_ID, local_vals.defSpecular, FALSE);
        if (drawableSpecularPrev_ID == -1) {
            gimp_message("Specular Edge Worker returned -1!");
            nResult = 0;
        } else nResult = 1;
        // nResult = specularEdge(image_ID, NULL, local_vals.defSpecular);
    }
    else
    {
        drawableSpecularPrev_ID = specularSmoothWorker(image_ID, local_vals.defSpecular, FALSE);
        if (drawableSpecularPrev_ID == -1) {
            gimp_message("Specular Smooth Worker returned -1!");
            nResult = 0;
        } else nResult = 1;
        // nResult = specularSmooth(image_ID, NULL, local_vals.defSpecular);
    }
    
    // removeGivenLayerFromImage(image_ID, temp_ID);
    // removeGivenLayerFromImage(image_ID, drawableDiffusePrev_ID);
    // removeGivenLayerFromImage(image_ID, drawableNormalPrev_ID);
    // removeGivenLayerFromImage(image_ID, drawableAOPrev_ID);
    if (nResult == 1) {
        return gimp_drawable_get(drawableSpecularPrev_ID);
    }
    return NULL;
}

/*  Public functions  */
/** Adding the preview area for the second release. */
void preview_redraw(void)
{
    if (!dialog_is_init) return;
    if (local_vals.prev == 1)
    {
        GimpDrawable *drawable = NULL;
        gint32 AOButton = FALSE;
        gint32 DButton = FALSE;
        gint32 SButton = FALSE;
        gint32 NButton = FALSE;
        
        _active = 0 ;
        
        if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(btn_ao)) == TRUE) {
            // modifies original so bad, original is not retrievable. 11:39pm
            drawable = preview_ambient_occlusion_only(local_vals.image_ID, TRUE);
            AOButton = TRUE;
        } else if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(btn_d)) == TRUE) {
            // works 11:39pm
            // This one had sub functions that when activated did not work.
            // That was fixed at 12:39pm
            drawable = preview_diffuse_only(local_vals.image_ID, TRUE);
            DButton = TRUE;
        } else if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(btn_s)) == TRUE) {
            // works 11:39pm
            drawable = preview_specular_only(local_vals.image_ID);
            SButton = TRUE;
        } else if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(btn_n)) == TRUE) {
            // modifies original so bad, original is not retrievable. 11:39pm
            // 11:48pm works
            // fixed by creating a copy of the original layer, modify that only.
            drawable = preview_normal_only(local_vals.image_ID, TRUE);
            NButton = TRUE;
        }

        draw_preview_area_update(drawable);

        if (AOButton) {
            removeAllLayersExceptMain();
        } else if (DButton) {
            removeAllLayersExceptMain();
        } else if (SButton) {
            removeAllLayersExceptMain();
        } else if (NButton) {
            removeAllLayersExceptMain();
        }
    }
}

void preview_clicked(GtkWidget *widget, gpointer data)
{
    if (!dialog_is_init) return;
    if (local_vals.prev == 0)
    {
        local_vals.prev = 1;
        gtk_button_set_label(GTK_BUTTON(widget), "Preview On");
        preview_redraw();
    }
    else
    {
        local_vals.prev = 0;
        gtk_button_set_label(GTK_BUTTON(widget), "Preview Off");

        // html color: #f2f1f0
        gimp_preview_area_fill(GIMP_PREVIEW_AREA(preview), 0, 0, PREVIEW_SIZE, PREVIEW_SIZE, 0xF2, 0xF1, 0xF0);

        _active = 0;
        return;
    }
}

/** Adding the preview area for the second release. */
void preview_clicked_occlusion(GtkWidget *widget, gpointer data)
{
    if (!dialog_is_init) return;
    if (local_vals.prev == 0) return;
    int nChecked = 0 ;
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)) == TRUE) nChecked = 1;
    
    if ((local_vals.prev == 1) && (nChecked == 1))
    {
        GimpDrawable *ao_drawable = preview_ambient_occlusion_only(local_vals.image_ID, TRUE);
        _active = 0 ;
        draw_preview_area_update(ao_drawable);
        removeAllLayersExceptMain();
    }
    else if ((local_vals.prev == 1) && (nChecked == 0))
    {
        return;
    }
}

/** Adding the preview area for the second release. */
void preview_clicked_diffuse(GtkWidget *widget, gpointer data)
{
    if (!dialog_is_init) return;
    if (local_vals.prev == 0) return;
    int nChecked = 0 ;
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)) == TRUE) nChecked = 1;
    
    if ((local_vals.prev == 1) && (nChecked == 1))
    {
        GimpDrawable *d_drawable = preview_diffuse_only(local_vals.image_ID, TRUE);
        _active = 0 ;
        draw_preview_area_update(d_drawable);
        removeAllLayersExceptMain();
    }
    else if ((local_vals.prev == 1) && (nChecked == 0))
    {
        return;
    }
}

/** Adding the preview area for the second release. */
void preview_clicked_specular(GtkWidget *widget, gpointer data)
{
    if (!dialog_is_init) return;
    if (local_vals.prev == 0) return;
    int nChecked = 0 ;
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)) == TRUE) nChecked = 1;
    
    if ((local_vals.prev == 1) && (nChecked == 1))
    {
        GimpDrawable *s_drawable = preview_specular_only(local_vals.image_ID);
        _active = 0 ;
        draw_preview_area_update(s_drawable);
        removeAllLayersExceptMain();
    }
    else if ((local_vals.prev == 1) && (nChecked == 0))
    {
        return;
    }
}

/** Adding the preview area for the second release. */
void preview_clicked_normal(GtkWidget *widget, gpointer data)
{
    if (!dialog_is_init) return;
    if (local_vals.prev == 0) return;
    int nChecked = 0 ;
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)) == TRUE) nChecked = 1;
    
    if ((local_vals.prev == 1) && (nChecked == 1))
    {
        GimpDrawable *n_drawable = preview_normal_only(local_vals.image_ID, TRUE);
        _active = 0 ;
        draw_preview_area_update(n_drawable);
        removeAllLayersExceptMain();
    }
    else if ((local_vals.prev == 1) && (nChecked == 0))
    {
        return;
    }
}

void removeAllLayersExceptMain(void) {
    gint *pnLayers = NULL;
    gint numLayers = 0 ;
    gint nIndex = 0 ;
    gimp_image_set_active_layer(local_vals.image_ID, drawableBeginActiveLayer);
    pnLayers = gimp_image_get_layers(local_vals.image_ID, &numLayers);
    
    for (nIndex=0;nIndex< numLayers;nIndex++) {
        if (pnLayers[nIndex] != drawableBeginActiveLayer) {
            if (gimp_layer_is_floating_sel(pnLayers[nIndex]))
            {
                gimp_floating_sel_remove(pnLayers[nIndex]);
           } else {
                gimp_image_remove_layer(local_vals.image_ID, pnLayers[nIndex]);
            }
        }
    }
}


int is_3D_preview_active(void)
{
   return(_active);
}

void CreatePreviewToggleButton(GtkWidget *hbox, GimpDrawable *drawable)
{
    GtkWidget *frame = NULL;
    GtkWidget *vbox = NULL;
    GtkWidget *abox = NULL;
    GtkWidget *btn = NULL;
    
    if (hbox == NULL) return;
    if (drawable == NULL) return;

    vbox = gtk_vbox_new(0, 8);
    gtk_box_pack_start(GTK_BOX(hbox), vbox, 1, 1, 0);
    gtk_widget_show(vbox);

    frame = gtk_frame_new("Preview");
    gtk_box_pack_start(GTK_BOX(vbox), frame, 0, 0, 0);
    gtk_widget_show(frame);

    abox = gtk_alignment_new(0.5, 0.5, 0.0, 0.0);
    gtk_container_set_border_width(GTK_CONTAINER (abox), 4);
    gtk_container_add(GTK_CONTAINER(frame), abox);
    gtk_widget_show(abox);

    frame = gtk_frame_new(NULL);
    gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_IN);
    gtk_container_add(GTK_CONTAINER(abox), frame);
    gtk_widget_show(frame);

    preview = gimp_preview_area_new();
    gimp_preview_area_set_max_size(GIMP_PREVIEW_AREA(preview), PREVIEW_SIZE, PREVIEW_SIZE);
    gtk_drawing_area_size(GTK_DRAWING_AREA(preview), PREVIEW_SIZE, PREVIEW_SIZE);
    gtk_container_add(GTK_CONTAINER(frame), preview);
    gtk_widget_show(preview);

    if (local_vals.prev == 1) {
        btn = gtk_toggle_button_new_with_label("Preview On");
    } else {
        btn = gtk_toggle_button_new_with_label("Preview Off");
    }
    if (btn != NULL)
    {
        // g_object_set_data(G_OBJECT(btn), "drawable", drawable);
    
        gtk_signal_connect(GTK_OBJECT(btn), "clicked",
                           GTK_SIGNAL_FUNC(preview_clicked), 0);

        gtk_box_pack_start(GTK_BOX(vbox), btn, 0, 0, 0);
        gtk_widget_show(btn);
        if (local_vals.prev == 1)
        {
           gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(btn), TRUE);
        }
    }
    
    btn_d = gtk_radio_button_new_with_label_from_widget(NULL, "Diffuse");
    if (btn_d != NULL)
    {
        // g_object_set_data(G_OBJECT(btn_d), "drawable", drawable);
    
        gtk_signal_connect(GTK_OBJECT(btn_d), "clicked",
                           GTK_SIGNAL_FUNC(preview_clicked_diffuse), 0);

        gtk_box_pack_start(GTK_BOX(vbox), btn_d, 0, 0, 0);
        gtk_widget_show(btn_d);
        // if ((local_vals.prev == 1) && (_diffuse_button == 1))
        // {
        
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(btn_d), TRUE);
        
        // }
    }
    
    if (btn_d == NULL) return;

    btn_ao = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(btn_d), "Occlusion");
    if (btn_ao != NULL)
    {
        // g_object_set_data(G_OBJECT(btn_ao), "drawable", drawable);
    
        gtk_signal_connect(GTK_OBJECT(btn_ao), "clicked",
                           GTK_SIGNAL_FUNC(preview_clicked_occlusion), 0);

        gtk_box_pack_start(GTK_BOX(vbox), btn_ao, 0, 0, 0);
        gtk_widget_show(btn_ao);
        // if ((local_vals.prev == 1) && (_occlusion_button == 1))
        // {
        //    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(btn_ao), TRUE);
        // }
    }

    btn_s = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(btn_d), "Specular");
    if (btn_s != NULL)
    {
        // g_object_set_data(G_OBJECT(btn_s), "drawable", drawable);
    
        gtk_signal_connect(GTK_OBJECT(btn_s), "clicked",
                           GTK_SIGNAL_FUNC(preview_clicked_specular), 0);

        gtk_box_pack_start(GTK_BOX(vbox), btn_s, 0, 0, 0);
        gtk_widget_show(btn_s);
        // if ((local_vals.prev == 1) && (_specular_button == 1))
        // {
        //    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(btn_s), TRUE);
        // }
    }

    btn_n = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(btn_d), "Normal");
    if (btn_n != NULL)
    {
        // g_object_set_data(G_OBJECT(btn_n), "drawable", drawable);
    
        gtk_signal_connect(GTK_OBJECT(btn_n), "clicked",
                           GTK_SIGNAL_FUNC(preview_clicked_normal), 0);

        gtk_box_pack_start(GTK_BOX(vbox), btn_n, 0, 0, 0);
        gtk_widget_show(btn_n);
        // if ((local_vals.prev == 1) && (_normal_button == 1))
        // {
        //    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(btn_n), TRUE);
        // }
    }

//    btn_h = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(btn_d), "Displacement");
//    if (btn_h != NULL)
//    {
//        gtk_signal_connect(GTK_OBJECT(btn_h), "clicked",
//                           GTK_SIGNAL_FUNC(preview_clicked_displacement), 0);
//
//        gtk_box_pack_start(GTK_BOX(vbox), btn_h, 0, 0, 0);
//        gtk_widget_show(btn_h);
//    }
//
//    btn_ln = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(btn_d), "low normal");
//    if (btn_ln != NULL)
//    {
//        gtk_signal_connect(GTK_OBJECT(btn_ln), "clicked",
//                           GTK_SIGNAL_FUNC(preview_clicked_lownormal), 0);
//
//        gtk_box_pack_start(GTK_BOX(vbox), btn_ln, 0, 0, 0);
//        gtk_widget_show(btn_ln);
//    }
}