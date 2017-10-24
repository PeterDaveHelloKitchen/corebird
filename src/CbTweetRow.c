/*  This file is part of corebird, a Gtk+ linux Twitter client.
 *  Copyright (C) 2017 Timm Bäder
 *
 *  corebird is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  corebird is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with corebird.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "CbTweetRow.h"
#include "CbUtils.h"
#include "corebird.h"

G_DEFINE_TYPE (CbTweetRow, cb_tweet_row, GTK_TYPE_LIST_BOX_ROW);


static void
cb_tweet_row_measure (GtkWidget      *widget,
                      GtkOrientation  orientation,
                      int             for_size,
                      int            *minimum,
                      int            *natural,
                      int            *minimum_baseline,
                      int            *natural_baseline)
{
  CbTweetRow *self = (CbTweetRow *)widget;

  if (orientation == GTK_ORIENTATION_HORIZONTAL)
    {
      /* Whatever, really. */

    }
  else /* VERTICAL */
    {
      int min = 0, nat = 0;
      guint i;
      GtkWidget* right_group[] = {
        self->top_row_box,
        self->reply_label,
        self->text_label,
        self->rt_image,
        self->mm_widget,
      };
      int avatar_width, left_height;

      gtk_widget_measure (self->avatar_widget, GTK_ORIENTATION_HORIZONTAL, -1,
                          &avatar_width, NULL, NULL, NULL);
      gtk_widget_measure (self->avatar_widget, GTK_ORIENTATION_VERTICAL, -1,
                          &left_height, NULL, NULL, NULL);

      for (i = 0; i < G_N_ELEMENTS (right_group); i ++)
        {
          int m, n;

          if (right_group[i])
            {

              gtk_widget_measure (right_group[i], GTK_ORIENTATION_VERTICAL,
                                  MAX (-1, for_size - avatar_width), &m, &n, NULL, NULL);

              min += m;
              nat += n;
            }
        }

      *minimum = MAX (left_height, min);
      *natural = MAX (left_height, nat);
    }
}

static void
cb_tweet_row_size_allocate (GtkWidget           *widget,
                            const GtkAllocation *allocation,
                            int                  baseline,
                            GtkAllocation       *out_clip)
{
  CbTweetRow *self = (CbTweetRow *)widget;
  GtkAllocation child_alloc;
  int min_width, nat_width;
  int min_height, nat_height;
  int avatar_width;
  int top_row_height;
  GtkAllocation child_clip;

  gtk_widget_measure (self->avatar_widget, GTK_ORIENTATION_HORIZONTAL, -1, &min_width, &nat_width, NULL, NULL);
  gtk_widget_measure (self->avatar_widget, GTK_ORIENTATION_VERTICAL, -1, &min_height, &nat_height, NULL, NULL);
  child_alloc.x = 0;
  child_alloc.y = 0;
  child_alloc.width = min_width;
  child_alloc.height = min_height;
  gtk_widget_size_allocate (self->avatar_widget, &child_alloc, -1, &child_clip);
  avatar_width = min_width;

  gtk_widget_measure (self->top_row_box, GTK_ORIENTATION_HORIZONTAL, -1, &min_width, &nat_width, NULL, NULL);
  gtk_widget_measure (self->top_row_box, GTK_ORIENTATION_VERTICAL, -1, &min_height, &nat_height, NULL, NULL);
  child_alloc.x += child_alloc.width;
  child_alloc.width = MAX (allocation->width - child_alloc.width, min_width);
  child_alloc.height = min_height;
  gtk_widget_size_allocate (self->top_row_box, &child_alloc, -1, &child_clip);
  top_row_height = min_height;

  if (self->reply_label != NULL)
    {
      gtk_widget_measure (self->reply_label, GTK_ORIENTATION_HORIZONTAL, -1, &min_width, &nat_width,
                          NULL, NULL);
      child_alloc.width = MAX (min_width, allocation->width - avatar_width);
      gtk_widget_measure (self->reply_label, GTK_ORIENTATION_VERTICAL, child_alloc.width,
                          &min_height, &nat_height, NULL, NULL);
      child_alloc.height = nat_height;
      child_alloc.y += top_row_height;
      gtk_widget_size_allocate (self->reply_label, &child_alloc, -1, &child_clip);
    }

  child_alloc.y += nat_height;
  gtk_widget_measure (self->text_label, GTK_ORIENTATION_HORIZONTAL, -1, &min_width, &nat_width, NULL, NULL);
  child_alloc.width = MAX (allocation->width - avatar_width, min_width);
  gtk_widget_measure (self->text_label, GTK_ORIENTATION_VERTICAL, child_alloc.width,
                      &min_height, &nat_height, NULL, NULL);
  child_alloc.x = avatar_width;
  child_alloc.height = nat_height;
  gtk_widget_size_allocate (self->text_label, &child_alloc, -1, &child_clip);

  if (self->rt_image != NULL)
    {
      int min_image_height, min_label_height;
      g_assert (self->rt_label != NULL);

      gtk_widget_measure (self->rt_image, GTK_ORIENTATION_VERTICAL, -1, &min_image_height, NULL,
                          NULL, NULL);
      gtk_widget_measure (self->rt_label, GTK_ORIENTATION_VERTICAL, -1, &min_label_height, NULL,
                          NULL, NULL);

      gtk_widget_measure (self->rt_image, GTK_ORIENTATION_HORIZONTAL, -1, &min_width, &nat_width,
                          NULL, NULL);
      child_alloc.x = avatar_width;
      child_alloc.y = child_alloc.y + child_alloc.height;
      child_alloc.height = MAX (min_image_height, min_label_height);
      child_alloc.width = min_width;
      gtk_widget_size_allocate (self->rt_image, &child_alloc, -1, &child_clip);

      gtk_widget_measure (self->rt_label, GTK_ORIENTATION_HORIZONTAL, -1, &min_width, &nat_width,
                          NULL, NULL);
      child_alloc.x += child_alloc.width;
      child_alloc.width = MAX (allocation->width - avatar_width, min_width);
      gtk_widget_size_allocate (self->rt_label, &child_alloc, -1, &child_clip);
    }

  if (self->mm_widget != NULL)
    {
      gtk_widget_measure (self->mm_widget, GTK_ORIENTATION_HORIZONTAL, -1, &min_width, &nat_width,
                          NULL, NULL);
      child_alloc.width = MAX (allocation->width - avatar_width, min_width);
      gtk_widget_measure (self->mm_widget, GTK_ORIENTATION_VERTICAL, child_alloc.width,
                          &min_height, &nat_height, NULL, NULL);
      child_alloc.x = avatar_width;
      child_alloc.y = child_alloc.y + child_alloc.height;
      child_alloc.height = min_height;
      gtk_widget_size_allocate (self->mm_widget, &child_alloc, -1, &child_clip);
    }
}

static void
cb_tweet_row_class_init (CbTweetRowClass *klass)
{
  GtkWidgetClass *widget_class = (GtkWidgetClass *)klass;

  widget_class->measure = cb_tweet_row_measure;
  widget_class->size_allocate = cb_tweet_row_size_allocate;
}

static void
cb_tweet_row_init (CbTweetRow *self)
{
}

static void
create_ui (CbTweetRow *self)
{
  g_assert (self->tweet != NULL);

  self->avatar_widget = (GtkWidget *)avatar_widget_new ();
  avatar_widget_set_verified (AVATAR_WIDGET (self->avatar_widget),
                              cb_tweet_is_flag_set (self->tweet, CB_TWEET_STATE_VERIFIED));
  gtk_widget_set_parent (self->avatar_widget, (GtkWidget *)self);
  self->top_row_box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_style_context_add_class (gtk_widget_get_style_context (self->top_row_box), "header");
  gtk_widget_set_parent (self->top_row_box, (GtkWidget *)self);

  twitter_get_avatar (twitter_get (), cb_tweet_get_user_id (self->tweet), self->tweet->avatar_url,
                      (AvatarWidget *)self->avatar_widget, 48, FALSE, NULL, NULL);

  self->name_button = (GtkWidget *)text_button_new ();
  text_button_set_markup ((TextButton*)self->name_button, cb_tweet_get_user_name (self->tweet));
  gtk_widget_set_valign (self->name_button, GTK_ALIGN_BASELINE);
  gtk_container_add (GTK_CONTAINER (self->top_row_box), self->name_button);

  self->screen_name_label = gtk_label_new (g_strdup_printf ("@%s", cb_tweet_get_screen_name (self->tweet)));
  gtk_style_context_add_class (gtk_widget_get_style_context (self->screen_name_label),
                               "dim-label");
  gtk_widget_set_hexpand (self->screen_name_label, TRUE);
  gtk_widget_set_halign (self->screen_name_label, GTK_ALIGN_START);
  gtk_widget_set_valign (self->screen_name_label, GTK_ALIGN_BASELINE);
  gtk_container_add (GTK_CONTAINER (self->top_row_box), self->screen_name_label);

  self->time_delta_label = gtk_label_new ("");
  gtk_widget_set_valign (self->time_delta_label, GTK_ALIGN_BASELINE);
  gtk_style_context_add_class (gtk_widget_get_style_context (self->time_delta_label), "dim-label");
  gtk_style_context_add_class (gtk_widget_get_style_context (self->time_delta_label), "time-delta");
  gtk_container_add (GTK_CONTAINER (self->top_row_box), self->time_delta_label);

  /* Reply label */
  if (self->tweet->source_tweet.reply_id != 0 ||
      (self->tweet->retweeted_tweet != NULL && self->tweet->retweeted_tweet->reply_id != 0))
    {
      GString *str = g_string_new (NULL);

      if (self->tweet->retweeted_tweet != NULL)
        cb_utils_write_reply_text (self->tweet->retweeted_tweet, str);
      else
        cb_utils_write_reply_text (&self->tweet->source_tweet, str);

      self->reply_label = gtk_label_new (g_string_free (str, FALSE)); // Still leaks
      gtk_label_set_xalign (GTK_LABEL (self->reply_label), 0.0f);
      gtk_label_set_yalign (GTK_LABEL (self->reply_label), 0.0f);
      gtk_label_set_use_markup (GTK_LABEL (self->reply_label), TRUE);
      gtk_label_set_track_visited_links (GTK_LABEL (self->reply_label), FALSE);
      gtk_label_set_line_wrap (GTK_LABEL (self->reply_label), TRUE);
      gtk_label_set_line_wrap_mode (GTK_LABEL (self->reply_label), PANGO_WRAP_WORD_CHAR);
      gtk_style_context_add_class (gtk_widget_get_style_context (self->reply_label),
                                   "dim-label");
      gtk_style_context_add_class (gtk_widget_get_style_context (self->reply_label),
                                   "invisible-links");
      gtk_widget_set_parent (self->reply_label, (GtkWidget *)self);
    }

  self->text_label = gtk_label_new (cb_tweet_get_trimmed_text (self->tweet, 0));
  gtk_label_set_xalign (GTK_LABEL (self->text_label), 0.0f);
  gtk_label_set_yalign (GTK_LABEL (self->text_label), 0.0f);
  gtk_label_set_use_markup (GTK_LABEL (self->text_label), TRUE);
  gtk_label_set_track_visited_links (GTK_LABEL (self->text_label), FALSE);
  gtk_label_set_line_wrap (GTK_LABEL (self->text_label), TRUE);
  gtk_label_set_line_wrap_mode (GTK_LABEL (self->text_label), PANGO_WRAP_WORD_CHAR);
  gtk_widget_set_parent (self->text_label, (GtkWidget *)self);

  /* Retweet indicators */
  if (self->tweet->retweeted_tweet != NULL)
    {
      self->rt_image = gtk_image_new_from_icon_name ("corebird-retweet-symbolic",
                                                     GTK_ICON_SIZE_MENU);
      gtk_style_context_add_class (gtk_widget_get_style_context (self->rt_image),
                                   "rt-icon");
      gtk_style_context_add_class (gtk_widget_get_style_context (self->rt_image),
                                   "dim-label");
      gtk_widget_set_parent (self->rt_image, (GtkWidget *)self);

      self->rt_label = gtk_label_new (self->tweet->source_tweet.author.user_name);
      gtk_style_context_add_class (gtk_widget_get_style_context (self->rt_label),
                                   "rt-label");
      gtk_style_context_add_class (gtk_widget_get_style_context (self->rt_label),
                                   "dim-label");
      gtk_widget_set_halign (self->rt_label, GTK_ALIGN_START);
      gtk_widget_set_parent (self->rt_label, (GtkWidget *)self);
    }

  /* Inline media */
  if (cb_tweet_has_inline_media (self->tweet))
    {
      int n_medias;
      CbMedia ** medias = cb_tweet_get_medias (self->tweet, &n_medias);
      self->mm_widget = (GtkWidget *)multi_media_widget_new ();
      gtk_widget_set_parent (self->mm_widget, (GtkWidget *)self);

      multi_media_widget_set_all_media ((MultiMediaWidget *)self->mm_widget, medias, n_medias);

      // TODO: Care about NSFW-ness
    }

  gtk_style_context_add_class (gtk_widget_get_style_context ((GtkWidget *)self), "tweet");

  cb_tweet_row_update_time_delta (self, NULL);
}

GtkWidget *
cb_tweet_row_new (CbTweet    *tweet)
                  /*MainWindow *main_window,*/
                  /*Account    *account)*/
{
  CbTweetRow *self  = (CbTweetRow *)g_object_new (CB_TYPE_TWEET_ROW, NULL);

  g_set_object (&self->tweet, tweet);
  create_ui (self);

  return (GtkWidget *)self;
}

void
cb_tweet_row_update_time_delta (CbTweetRow *self,
                                GDateTime  *now)
{
  GDateTime *cur_time = now != NULL ? g_date_time_ref (now) :
                                      g_date_time_new_now_local ();
  GDateTime *then;
  char *delta_str;

  then = g_date_time_new_from_unix_local (self->tweet->retweeted_tweet != NULL ?
                                          self->tweet->retweeted_tweet->created_at :
                                          self->tweet->source_tweet.created_at);

  delta_str = cb_utils_get_time_delta (then, cur_time);
  gtk_label_set_label (GTK_LABEL (self->time_delta_label), delta_str);
  /* XXX Incomplete: Quotes */

  g_free (delta_str);
}