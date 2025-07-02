import { Module } from '@nestjs/common';
import { AppController } from './app.controller';
import { AppService } from './app.service';
import { UsersService } from './users/users.service';
import { AuthModule } from './auth/auth.module';
import { UsersModule } from './users/users.module';
import { ConfigModule, ConfigService } from '@nestjs/config';
import { DevicesModule } from './devices/devices.module';
import { MqttModule } from './mqtt/mqtt.module';
import { WebsocketGateway } from './websocket/websocket.gateway';
import { WebsocketModule } from './websocket/websocket.module';
import { TypeOrmModule } from '@nestjs/typeorm';
import { User } from './users/user.entity';
import { Device } from './devices/device.entity';
import { DbLoggerService } from './database/db-logger.service';

@Module({
  imports: [ 
    AuthModule, 
    UsersModule,
    
    ConfigModule.forRoot({
      isGlobal: true,
    }),
    
    TypeOrmModule.forRootAsync({
      imports: [ConfigModule],
      inject: [ConfigService],
      useFactory: (config: ConfigService) => ({
        type: 'postgres',
        host: config.get('DB_HOST'),
        port: config.get<number>('DB_PORT'),
        username: config.get('DB_USERNAME'),
        password: config.get('DB_PASSWORD'),
        database: config.get('DB_NAME'),
        entities: [User, Device], // Telemetry Later
        synchronize: true,
        logging: true,
      }),
    }),
    TypeOrmModule.forFeature([User, Device]),

    DevicesModule,
    MqttModule,
    WebsocketModule ],
  controllers: [AppController],
  providers: [AppService, UsersService, WebsocketGateway, DbLoggerService],
})
export class AppModule {}
