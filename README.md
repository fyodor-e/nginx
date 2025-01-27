# Тестовое задание

Исходный код nginx с моими изменениями лежит [здесь](https://github.com/fyodor-e/nginx/tree/test-task?tab=readme-ov-file) в ветке test-task. Это fork официального репозитория nginx.

Все изменения в одном комите. Посмотреть их можно по [ссылке](https://github.com/nginx/nginx/compare/master...fyodor-e:nginx:test-task). На изменение файла README.md не обращайте внимания.

Разработка велась на Ubuntu 22.04.3 LTS

### Билд и запуск

```
  git clone https://github.com/fyodor-e/nginx
  cd nginx
  git switch test-task
  auto/configure
  make
  make install
  mkdir /nginx-debug
  chmod 777 /nginx-debug
```

Для запуска nginx использовался следюущий файл конфигурации `/usr/local/nginx/conf/nginx.conf`

```
worker_processes  1;

events {
    worker_connections  1024;
}


http {
    include       mime.types;
    default_type  application/octet-stream;
    
    # формат лога после обработки запроса (текущая реализация)
    log_format main '$remote_addr - $remote_user [$time_local] '
                       '"$request" $status $bytes_sent '
                       '"$http_referer" "$http_user_agent" "$gzip_ratio" "$test_header_name"';

    # формат лога в момент получения запроса (добавлено мной)                       
    log_format_pre main_pre '$remote_addr - $remote_user [$time_local] '
                       '"$request" $method $protocol_version $test_header_name';

    sendfile        on;
    keepalive_timeout  65;

    server {
        listen       80;
        server_name  localhost;

        location / {
            root   html;
            index  index.html index.htm;
            # Имя тестового заголовка
   	        test_header_name test-header-iii;
   	        access_log /nginx-debug/access.log main buffer=32k;
            access_log /nginx-debug/access_pre.log main_pre buffer=32k;
        }

        error_page   500 502 503 504  /50x.html;
        location = /50x.html {
            root   html;
        }
    }
}
```

После настройки конфигурации запускаем nginx и делаем тестовый запрос

```
  /usr/local/nginx/sbin/nginx
  curl -i localhost
  /usr/local/nginx/sbin/nginx -s stop
```

В файлах будут логи `/nginx-debug/access_pre.log` и `/nginx-debug/access.log`. В первом лог в момент получения запроса, во втором - после обработки HTTP запроса.

### Особенности

 - Запись в лог в момент получения запроса идет на стадии `NGX_HTTP_PREACCESS_PHASE`, когда известна location
 - Поле `name` в `log_format` и `log_format_pre` должны различаться (я не делал проверки уникальности имени). Также файлы для записи лога `access_log` должны быть разными для лога в момент получения запроса и для лога после обработки запроса.
 - В `log_format_pre` добавлены 2 переменные `$method` и `$protocol_version`
 - В лог `log_format_pre` перед каждой строкой лога записывается слово `PRE: ` для того, чтобы видеть что это лог входящего запроса