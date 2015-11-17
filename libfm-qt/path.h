/*
 * Copyright (C) 2014  Hong Jen Yee (PCMan) <pcman.tw@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */

#ifndef FM_PATH_H
#define FM_PATH_H

#include "libfmqtglobals.h"
#include <libfm/fm.h>
#include <QString>
#include <QMetaType>

namespace Fm {

class LIBFM_QT_API Path {
public:

  Path(): data_(nullptr) {
  }

  Path(FmPath* path, bool takeOwnership = false): data_(path) {
    if(path && !takeOwnership)
      fm_path_ref(data_);
  }

  Path(const Path& other): data_(other.data_ ? fm_path_ref(other.data_) : nullptr) {
  }

  Path(GFile* gf): data_(fm_path_new_for_gfile(gf)) {
  }

  ~Path() {
    if(data_)
      fm_path_unref(data_);
  }

  static Path fromPathName(const char* path_name) {
    return Path(fm_path_new_for_path(path_name), true);
  }

  static Path fromUri(const char* uri) {
    return Path(fm_path_new_for_uri(uri), true);
  }

  static Path fromDisplayName(const char* path_name) {
    return Path(fm_path_new_for_display_name(path_name), true);
  }

  static Path fromString(const char* path_str) {
    return Path(fm_path_new_for_str(path_str), true);
  }

  static Path fromCommandlineArg(const char* arg) {
    return Path(fm_path_new_for_commandline_arg(arg), true);
  }

  Path child(const char* basename) {
    return Path(fm_path_new_child(data_, basename), true);
  }

  Path child(const char* basename, int name_len) {
    return Path(fm_path_new_child_len(data_, basename, name_len), true);
  }

  Path relative(const char* rel) {
    return Path(fm_path_new_relative(data_, rel), true);
  }

  /* predefined paths */
  static Path root(void) { /* / */
    return Path(fm_path_get_root(), false);
  }

  static Path home(void) { /* home directory */
    return Path(fm_path_get_home(), false);
  }

  static Path desktop(void) { /* $HOME/Desktop */
    return Path(fm_path_get_desktop(), false);
  }

  static Path trash(void) { /* trash:/// */
    return Path(fm_path_get_trash(), false);
  }

  static Path appsMenu(void) { /* menu://applications.menu/ */
    return Path(fm_path_get_apps_menu(), false);
  }

  Path parent() {
    return Path(data_ != nullptr ? fm_path_get_parent(data_) : nullptr, false);
  }

  const char* basename() {
    return fm_path_get_basename(data_);
  }

  FmPathFlags flags() {
    return fm_path_get_flags(data_);
  }

  bool hasPrefix(FmPath* prefix) {
    return fm_path_has_prefix(data_, prefix);
  }

  Path schemePath() {
    return Path(fm_path_get_scheme_path(data_), true);
  }

  bool isNative() {
    return fm_path_is_native(data_);
  }

  bool isTrash() {
    return fm_path_is_trash(data_);
  }

  bool isTrashRoot() {
    return fm_path_is_trash_root(data_);
  }

  bool isNativeOrTrash() {
    return fm_path_is_native_or_trash(data_);
  }

  char* toString() {
    return fm_path_to_str(data_);
  }

  QByteArray toByteArray() {
    char* s = fm_path_to_str(data_);
    QByteArray str(s);
    g_free(s);
    return str;
  }

  char* toUri() {
    return fm_path_to_uri(data_);
  }

  GFile* toGfile() {
    return fm_path_to_gfile(data_);
  }

  /*
  char* displayName(bool human_readable = true) {
    return fm_path_display_name(data_, human_readable);
  }
  */

  QString displayName(bool human_readable = true) {
    char* dispname = fm_path_display_name(data_, human_readable);
    QString str = QString::fromUtf8(dispname);
    g_free(dispname);
    return str;
  }

  /*
  char* displayBasename() {
    return fm_path_display_basename(data_);
  }
  */

  QString displayBasename() {
    char* basename = fm_path_display_basename(data_);
    QString s = QString::fromUtf8(basename);
    g_free(basename);
    return s;
  }

  /* For used in hash tables */
  guint hash() {
    return fm_path_hash(data_);
  }

  void take(FmPath* path) { // take the ownership of the "path"
    if(data_)
      fm_path_unref(data_);
    data_ = path;
  }

  Path& operator = (const Path& other) {
    if(data_)
      fm_path_unref(data_);
    data_ = fm_path_ref(other.data_);
    return *this;
  }

  bool operator == (const Path& other) const {
    return fm_path_equal(data_, other.data_);
  }

  bool operator != (const Path& other) const {
    return !fm_path_equal(data_, other.data_);
  }

  bool operator < (const Path& other) const {
    return compare(other);
  }

  bool operator > (const Path& other) const {
    return (other < *this);
  }

  /* can be used for sorting */
  int compare(const Path& other) const {
    return fm_path_compare(data_, other.data_);
  }

  /* used for completion in entry */
  bool equal(const gchar *str, int n) const {
    return fm_path_equal_str(data_, str, n);
  }

  /* calculate how many elements are in this path. */
  int depth() const {
    return fm_path_depth(data_);
  }

  FmPath* data() const {
    return data_;
  }

private:
  FmPath* data_;
};

}

Q_DECLARE_OPAQUE_POINTER(FmPath*)

#endif // FM_PATH_H
