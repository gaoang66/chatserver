aux_source_directory(. SRC_LIST)
aux_source_directory(./db DB_LIST)
aux_source_directory(./model MD_LIST)
aux_source_directory(./redis REDIS_LIST)

add_executable(ChatServer ${DB_LIST} ${SRC_LIST} ${MD_LIST} ${REDIS_LIST})

target_link_libraries(ChatServer
    mysqlclient
    muduo_net
    muduo_base
    hiredis
    pthread
)
