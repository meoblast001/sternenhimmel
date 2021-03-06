/**
 * Copyright (C) 2017 Braden Walters
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

extern "C" {
  #include <mkdio.h>
}

#include <QNetworkRequest>
#include <QJsonArray>
#include <QJsonObject>
#include <QRegularExpression>
#include "Diaspora.hpp"

Diaspora::Diaspora(const QString& scheme, const QString& podHost)
{
  podUrl.setScheme(scheme);
  podUrl.setHost(podHost);
}

const QUrl& Diaspora::getPodUrl() const
{
  return podUrl;
}

void Diaspora::fetchPosts(const QString& tag)
{
  // Construct specific URL for tag's posts.
  QUrl url = podUrl;
  url.setPath(QString("/tags/") + tag + QString(".json"));
  // Create network request.
  activeReply = networkManager.get(QNetworkRequest(url));
  connect(activeReply, SIGNAL(finished()), this, SLOT(httpFinished()));
  connect(activeReply, SIGNAL(error(QNetworkReply::NetworkError)),
          this, SLOT(httpError(QNetworkReply::NetworkError)));
  connect(activeReply, SIGNAL(sslErrors(QList<QSslError>)),
          this, SLOT(httpSslError()));
}

list<PostEntity> Diaspora::parseJson(const QJsonDocument& json)
  throw (ParseException)
{
  // Must be an array to be valid.
  if (!json.isArray()) {
    throw ParseException("Root element is not an array.");
  }

  // Begin each array object.
  auto entities = list<PostEntity>();
  QJsonArray jsonPosts = json.array();
  for (auto iPost = jsonPosts.begin(); iPost != jsonPosts.end(); ++iPost) {
    if (iPost->isObject()) {
      PostEntity entity = parsePostJson(iPost->toObject());
      entities.push_back(entity);
    } else {
      throw ParseException("Post must be an object.");
    }
  }
  return entities;
}

QString Diaspora::prepareMarkdown(const QString& markdown)
{
  QString result(markdown);
  return result.replace(QRegularExpression("(^|\\s)#([\\w\\d]+)"),
                        QString("\\1&#x23;\\2"));
}

QString Diaspora::parseMarkdown(const QString& markdown) throw (ParseException)
{
  auto mdByteArr = markdown.toLocal8Bit();
  MMIOT* mdDoc = mkd_string(mdByteArr.data(), mdByteArr.size(), 0);
  char* resultCStr;
  if (!mkd_compile(mdDoc, 0)) {
    mkd_cleanup(mdDoc);
    throw ParseException("Cannot compile markdown.");
  }
  int resultSize = mkd_document(mdDoc, &resultCStr);
  QByteArray resultByteArr(resultCStr, resultSize);
  QString result(resultByteArr);
  mkd_cleanup(mdDoc);
  return result;
}

PostEntity Diaspora::parsePostJson(const QJsonObject& json)
  throw (ParseException)
{
  // Attempt to parse.
  auto text = json.value("text").toString();
  if (text.isNull()) {
    throw ParseException("Post must contain text.");
  }
  auto authorObj = json.value("author").toObject();
  if (authorObj.isEmpty()) {
    throw ParseException("Post author must be an object.");
  }
  auto authorName = authorObj.value("name").toString();
  if (authorName.isNull()) {
    throw ParseException("Post author must contain a name.");
  }
  auto authorAvatarObj = authorObj.value("avatar").toObject();
  if (authorAvatarObj.isEmpty()) {
    throw ParseException("Post author avatar must be an object.");
  }
  auto authorAvatarMedium = authorAvatarObj.value("medium").toString();
  if (authorAvatarMedium.isNull()) {
    throw ParseException("Post author medium avatar must contain a URL.");
  }
  // Return successful result.
  return PostEntity(authorName, authorAvatarMedium, text);
}

void Diaspora::httpFinished()
{
  if (NULL == activeReply || QNetworkReply::NoError != activeReply->error()) {
    return;
  }
  auto response = activeReply->readAll();
  auto json = QJsonDocument::fromJson(response);
  try {
    auto entities = parseJson(json);
    for (auto entity = entities.begin(); entity != entities.end(); ++entity) {
      entity->setText(parseMarkdown(prepareMarkdown(entity->getText())));
    }
    emit finished(entities);
  } catch (ParseException& e) {
    emit error(e.what());
  }
}

void Diaspora::httpError(QNetworkReply::NetworkError code)
{
  const char* message;
  switch (code) {
    case QNetworkReply::ConnectionRefusedError:
      message = "Connection refused.";
      break;
    case QNetworkReply::RemoteHostClosedError:
      message = "Remote host closed.";
      break;
    case QNetworkReply::HostNotFoundError:
      message = "Host not found.";
      break;
    default:
      message = "Other.";
      break;
  }
  emit error(message);
}

void Diaspora::httpSslError()
{
  emit error("SSL error.");
}
